#include "DataStorage.h"

#define CS_PIN 5 

/***
 * 函数功能：将 "{0x00, 0x00, ...,}" 格式字符串转换为 uint8_t 数组
 * 输   入：str: 输入的字符串
 * 输   出：uint8_t 数组；true: 转换成功；false: 转换失败
 */
bool strToUint8Array(String& str, std::vector<uint8_t>& font) 
{
  // 1. 预处理：去除字符串中的 '{'、'}' 和空格
  String& cleanStr = str;
  if (cleanStr.isEmpty()) Serial.println("1:传入字符为空");

  cleanStr.replace("{", "");   // 移除左大括号
  if (cleanStr.isEmpty()) Serial.println("2:移除大括号之后为空");

  cleanStr.replace(",}", "");   // 移除右大括号
  if (cleanStr.isEmpty()) Serial.println("3:移除右大括号为空");

  cleanStr.replace(" ", "");   // 移除所有空格
  if (cleanStr.isEmpty()) 
  {
    Serial.println("错误：预处理后字符串为空");
    return false;
  }

  // 2. 拆分字符串：以 ',' 为分隔符，提取每个 0xXX 子串
  int startIdx = 0;            // 每个子串的起始索引
  int commaIdx = cleanStr.indexOf(',');  // 查找第一个 ',' 的位置

  while (true) 
  {
    // 提取当前子串（从 startIdx 到 commaIdx，或到字符串末尾）
    String hexSubStr;
    if (commaIdx == -1) 
    { hexSubStr = cleanStr.substring(startIdx); } 
    else {hexSubStr = cleanStr.substring(startIdx, commaIdx);}

    // 3. 验证子串格式：必须以 "0x" 开头（十六进制标识）
    if (hexSubStr.length() < 4 || hexSubStr.substring(0, 2) != "0x") 
    {
      Serial.print("错误：无效的十六进制格式 -> ");
      Serial.println(hexSubStr);
      continue;
    }

    // 4. 将 0xXX 字符串转换为 uint8_t 数值
    char hexCharBuf[10];
    hexSubStr.toCharArray(hexCharBuf, sizeof(hexCharBuf));  // String 转 char[]
    unsigned long hexValue = strtoul(hexCharBuf, NULL, 16);  // 十六进制转长整型

    // 验证数值范围：uint8_t 只能存储 0~255
    if (hexValue > 0xFF) 
    {
      Serial.print("错误：数值超出 uint8_t 范围（0~255） -> ");
      Serial.println(hexValue);
      return false;
    }

    // 5. 存入 vector（自动扩容，无需手动管理数组长度）
    font.push_back(static_cast<uint8_t>(hexValue));

    // 6. 更新索引，处理下一个子串
    if (commaIdx == -1) 
    {  
      break;
    }
    startIdx = commaIdx + 1;   // 下一个子串的起始索引（跳过当前 ','）
    commaIdx = cleanStr.indexOf(',', startIdx);  // 查找下一个 ','
  }
  return true;
}



/***
 * 函数功能：从文件中读取一行（以'\n'为结束符）
 * 输   入：对文件的引用
 * 输   出：读取的字符串
 * 最近修改：2025.9.13
***/ 
String readLineFromFile(File &file) 
{
  String line = "";  // 存储读取到的一行内容
  while (file.available()) 
  {  
    char c = file.read(); 
    
    // 遇到换行符'\n'，结束当前行的读取
    if (c == '\n') { break; }
    
    // 忽略回车符'\r'（兼容Windows格式的换行\r\n）
    if (c != '\r') 
    {
      line += c;  // 拼接字符到字符串
    }
  }
  return line; 
}


SDStorage::SDStorage()
{

}



/***
 * 函数功能：SD卡初始化
 * 输   出：初始化结果
 * 最近修改：2025.9.13
 */
bool SDStorage::SD_Init()
{
    if (!SD.begin(CS_PIN)) 
    {  
        Serial.println("初始化失败！");
        return false;  // 初始化失败则退出
    }else return true;
}




/***
 * 函数功能：从文件中读取字体数据
 * 最近修改：2025.9.17
 */
std::vector<std::vector<uint8_t>> SDStorage::SD_ReadFont(String path)
{
    File myFile;
    std::vector<std::vector<uint8_t>> fontVector;

    // 打开文件（只读模式）
    myFile = SD.open(path); 
    if (myFile) 
    {
        Serial.println("打开文件成功！");
        String hexStr = "";
        while(hexStr = readLineFromFile(myFile))
        {
            if(!hexStr.isEmpty())
            {
                std::vector<uint8_t> fontArray;
                bool success = strToUint8Array(hexStr, fontArray);
                if (!success) 
                {
                    Serial.println("转换失败！");
                    continue;
                }else fontVector.push_back(fontArray);
            }else break;
        }
    
        // 关闭文件
        myFile.close();
        Serial.println("\n文件已关闭");
        return fontVector;
    } else 
    {
        Serial.println("文件打开失败！");
        return fontVector;
    }
}
