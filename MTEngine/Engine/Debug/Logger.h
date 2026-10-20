#pragma once
#include <Windows.h>
#include <string>
#include<format>
#include<filesystem>
#include<fstream>
#include<chrono>


namespace MTEngine {

	// 指定されたストリームにメッセージを出力する
	void Log(std::ostream& os, const std::string& message);

	// 標準出力へメッセージを出力する
	void Log(const std::string& message);

	// 文字列変換ヘルパー（std::string から std::wstring）
	std::wstring ConvertString(const std::string& str);

	// 文字列変換ヘルパー（std::wstring から std::string）
	std::string ConvertString(const std::wstring& wstr);

	// ロガーシステムの初期化
	void InitializeLogger();

	// ログ用ファイルの作成
	void createLogFile();

}