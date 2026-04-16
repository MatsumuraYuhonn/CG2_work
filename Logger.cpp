#include"Logger.h"

// ログを出力する関数
void Log(std::ostream& os, const std::string& message) {

	os << message << std::endl;

	OutputDebugStringA(message.c_str());
}


void InitializeLogger() {

	// ログのディレクトリを用意
	std::filesystem::create_directory("logs");

}


std::wstring ConvertString(const std::string& str) {
    if (str.empty()) {
        return std::wstring();
    }

    auto sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), NULL, 0);
    if (sizeNeeded == 0) {
        return std::wstring();
    }
    std::wstring result(sizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), &result[0], sizeNeeded);
    return result;
}

std::string ConvertString(const std::wstring& str) {
    if (str.empty()) {
        return std::string();
    }

    auto sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0, NULL, NULL);
    if (sizeNeeded == 0) {
        return std::string();
    }
    std::string result(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), sizeNeeded, NULL, NULL);
    return result;
}


void createLogFile()
{

	// 現在時刻を取得
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

	// 秒単位に変換
	std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds> nowSeconds = std::chrono::time_point_cast<std::chrono::seconds>(now);

	// 日本時間に変換
	std::chrono::zoned_time localTime{ std::chrono::current_zone(), nowSeconds };

	// 年月日_時分秒の形式で文字列に変換
    std::string dateString = std::format("{:%Y%m%d_%H%M%S}", localTime);

	// ログファイルのパスを作成
    std::string logFilePath = std::string("logs/") + dateString + ".log";

	// ログファイルを作成
    std::ofstream logStream(logFilePath);

}