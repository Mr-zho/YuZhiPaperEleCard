const ecBLE = require('../../utils/ecBLE.js');

// ==========================================
// 辅助函数：将 JS 字符串手动转换为 UTF-8 字节数组
// ==========================================
function stringToUtf8Bytes(str) {
  const bytes = [];
  for (let i = 0; i < str.length; i++) {
    let code = str.charCodeAt(i);
    if (code < 0x80) {
      bytes.push(code);
    } else if (code < 0x800) {
      bytes.push(0xc0 | (code >> 6), 0x80 | (code & 0x3f));
    } else if (code < 0xd800 || code >= 0xe000) {
      bytes.push(0xe0 | (code >> 12), 0x80 | ((code >> 6) & 0x3f), 0x80 | (code & 0x3f));
    } else {
      i++;
      code = 0x10000 + (((code & 0x3ff) << 10) | (str.charCodeAt(i) & 0x3ff));
      bytes.push(0xf0 | (code >> 18), 0x80 | ((code >> 12) & 0x3f), 0x80 | ((code >> 6) & 0x3f), 0x80 | (code & 0x3f));
    }
  }
  return new Uint8Array(bytes);
}

Page({
  data: {
    inputText: "",
    isSending: false
  },

  onInput(e) {
    this.setData({ inputText: e.detail.value });
  },

  async onSend() {
    if (this.data.isSending) return;

    const inputText = this.data.inputText.trim();
    if (!inputText) {
      wx.showToast({ title: "请输入内容", icon: "none" });
      return;
    }

    this.setData({ isSending: true });
    wx.showLoading({ title: '正在发送...', mask: true });

    try {
      // ==========================================
      // 核心修改：调用上面的手写函数完成 UTF-8 转换
      // ==========================================
      const payloadBytes = stringToUtf8Bytes(inputText);
      const payloadLen = payloadBytes.length;

      // 2. 计算总包长 = Header(12字节) + Payload 长度
      // 对应 C++ ProtoHeader 结构
      const headerLen = 12;
      const buffer = new ArrayBuffer(headerLen + payloadLen);
      const view = new DataView(buffer);

      // 3. 严格按照 C++ struct 顺序及字节大小写入数据 (Arduino 默认为小端序 Little-Endian)
      view.setUint8(0, 0xA5);          // magic (PROTO_MAGIC)
      view.setUint8(1, 0x01);          // version (PROTO_VERSION)
      view.setUint8(2, 0x02);          // type (TYPE_TEXT = 0x02)
      view.setUint8(3, 0x00);          // flags

      view.setUint16(4, 0, true);      // seq = 0 (当前包序号，小端序)
      view.setUint16(6, 1, true);      // total = 1 (总包数 = 1，表示不分片直接发送，小端序)

      view.setUint32(8, payloadLen, true); // payload_len (真实数据长度，4字节，小端序)

      // 4. 将文本的实际内容(Payload)复制到 Header 后面
      const u8Array = new Uint8Array(buffer);
      u8Array.set(payloadBytes, headerLen);

      // 5. 最终将打包好的 ArrayBuffer 送入蓝牙发送函数
      await ecBLE.sendUint8Array(u8Array);
      
      wx.hideLoading();
      wx.showToast({ title: "发送成功", icon: "success" });
      this.setData({ inputText: "" }); 
    } catch (err) {
      console.error('蓝牙发送失败：', err);
      wx.hideLoading();
      wx.showToast({ title: "发送失败", icon: "none" });
    } finally {
      this.setData({ isSending: false });
    }
  }
});