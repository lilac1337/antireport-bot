#pragma once

#include <dpp/dpp.h>
#include <dpp/snowflake.h>
#include <dpp/webhook.h>
#include <vector>
#include <exception>

class ConfigValueException : public std::exception {
private:
    std::string_view value;

public:
    ConfigValueException(std::string val) : value(val) {}

    const char *what() const noexcept override {
        return value.data();
    }
};

namespace report::bot {
	inline std::string botToken{};

    inline std::unique_ptr<dpp::cluster> bot{ nullptr };
    
    inline std::vector<dpp::webhook> webhooks;
    inline std::vector<std::string> words;

    [[nodiscard]] std::string getNickname(const dpp::snowflake guildId, const dpp::snowflake userId) noexcept;
    [[nodiscard]] bool shouldDelete(const std::string &message) noexcept;
	void handleMessage(const dpp::message_create_t &event) noexcept;
    
    void loadChannelConfig(const std::string &configPath);
    void loadWordConfig(const std::string &configPath);
    void loadTokenConfig(const std::string &configPath);
    
	void handler() noexcept;
};
