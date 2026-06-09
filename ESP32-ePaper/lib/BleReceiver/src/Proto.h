#pragma once
#include <Arduino.h>
#include <vector>
#include <map>

#define PROTO_MAGIC 0xA5
#define PROTO_VERSION 0x01

#define PROTO_TIMEOUT 5000   // 分片超时(ms)
#define PROTO_MAX_SESSIONS 4 // 最大同时重组数量（防内存炸）

enum ProtoType {
    TYPE_TEXT  = 0x02,
    TYPE_IMAGE = 0x03
};

#pragma pack(push,1)
struct ProtoHeader {
    uint8_t magic;
    uint8_t version;
    uint8_t type;
    uint8_t flags;

    uint16_t seq;
    uint16_t total;

    uint32_t payload_len;
};
#pragma pack(pop)

#pragma pack(push,1)
struct ImageHeader {
    uint16_t width;
    uint16_t height;
    uint8_t format;
};
#pragma pack(pop)


// ============================
// 分片重组结构
// ============================
struct ReassemblyBuffer {
    uint16_t total;
    uint8_t type;

    std::vector<std::vector<uint8_t>> frags;
    std::vector<bool> received;

    uint32_t receivedCount = 0;
    uint32_t totalBytes = 0;

    uint32_t lastUpdateMs = 0;
};


class ProtoBase {
public:
    virtual ~ProtoBase() {}

    void input(const uint8_t* data, size_t len)
    {
        buffer.insert(buffer.end(), data, data + len);

        while (buffer.size() >= sizeof(ProtoHeader)) {

            ProtoHeader hdr;
            memcpy(&hdr, buffer.data(), sizeof(ProtoHeader));

            if (hdr.magic != PROTO_MAGIC) {
                buffer.erase(buffer.begin());
                continue;
            }

            size_t totalSize = sizeof(ProtoHeader) + hdr.payload_len;

            if (buffer.size() < totalSize)
                return;

            const uint8_t* payload = buffer.data() + sizeof(ProtoHeader);

            handleFragment(hdr, payload);

            buffer.erase(buffer.begin(), buffer.begin() + totalSize);
        }

        cleanupTimeout();
    }

protected:
    virtual void dispatch(uint8_t type,
                          const uint8_t* payload,
                          size_t len) = 0;

private:
    std::vector<uint8_t> buffer;

    std::map<uint32_t, ReassemblyBuffer> sessions;

    // ============================
    // session key
    // ============================
    uint32_t makeKey(const ProtoHeader& hdr)
    {
        // 简化：用 type + total 做key（实际项目建议增加 msg_id）
        return (hdr.type << 16) | hdr.total;
    }

    void handleFragment(const ProtoHeader& hdr, const uint8_t* payload)
    {
        if (hdr.total <= 1) {
            dispatch(hdr.type, payload, hdr.payload_len);
            return;
        }

        uint32_t key = makeKey(hdr);

        auto& sess = sessions[key];

        if (sess.frags.empty()) {
            sess.total = hdr.total;
            sess.type = hdr.type;

            sess.frags.resize(hdr.total);
            sess.received.resize(hdr.total, false);
        }

        if (hdr.seq >= sess.total) return;

        if (!sess.received[hdr.seq]) {
            sess.frags[hdr.seq] = std::vector<uint8_t>(payload, payload + hdr.payload_len);
            sess.received[hdr.seq] = true;
            sess.receivedCount++;
            sess.totalBytes += hdr.payload_len;
        }

        sess.lastUpdateMs = millis();

        // ============================
        // 判断是否完整
        // ============================
        if (sess.receivedCount == sess.total) {

            std::vector<uint8_t> merged;
            merged.reserve(sess.totalBytes);

            for (uint16_t i = 0; i < sess.total; ++i) {
                merged.insert(merged.end(),
                              sess.frags[i].begin(),
                              sess.frags[i].end());
            }

            dispatch(sess.type, merged.data(), merged.size());

            sessions.erase(key);
        }

        // 防止 session 爆炸
        if (sessions.size() > PROTO_MAX_SESSIONS) {
            sessions.erase(sessions.begin());
        }
    }

    void cleanupTimeout()
    {
        uint32_t now = millis();

        for (auto it = sessions.begin(); it != sessions.end(); ) {
            if (now - it->second.lastUpdateMs > PROTO_TIMEOUT) {
                it = sessions.erase(it);
            } else {
                ++it;
            }
        }
    }
};