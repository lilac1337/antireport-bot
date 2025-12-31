#include "botHandler.hxx"

#include <dpp/appcommand.h>
#include <dpp/dispatcher.h>
#include <dpp/misc-enum.h>
#include <fstream>

[[nodiscard]] constexpr std::string toLower(std::string str) noexcept {
    for (char &c : str) {
        if (c >= 'A' && c <= 'Z') {
            c += 'a' - 'A';
        }
    }

    return str;
}

[[nodiscard]] bool report::bot::shouldDelete(const std::string &message) noexcept {
    std::string lower = toLower(message);

    for (std::string &w : words)
        if (lower.contains(w))
            return true;
    
    return false;
}

void report::bot::handleMessage(const dpp::message_create_t &event) noexcept {
    const std::string &message = event.msg.content;

    if (!shouldDelete(message) || !event.msg.attachments.empty())
        return;

    const dpp::snowflake messageChannelID = event.msg.channel_id;

    auto it = std::find_if(webhooks.begin(), webhooks.end(), [&messageChannelID](const dpp::webhook& wh) {
        return messageChannelID == wh.channel_id;
    });

    if (it != webhooks.end()) {
        it->avatar_url = event.msg.author.get_avatar_url();
        it->name = event.msg.author.global_name;

        bot->message_delete(event.msg.id, messageChannelID);
        bot->execute_webhook(*it, dpp::message(message));
    }
}

void report::bot::loadChannelConfig(const std::string configPath) {
    std::fstream stream{ configPath, stream.in };
    
    if (!stream.is_open())
        throw ConfigValueException("Couldn't load channel config.");

    for (std::string line; std::getline(stream, line);) {
        dpp::webhook wh;

        if (line.empty() || line.at(0) == '#')
            continue;

        for (auto word : std::views::split(line, ',')) {
            if (wh.token.empty()) {
                wh = dpp::webhook(std::string(std::string_view(word)));
                continue;
            }

            wh.channel_id = std::stoull(word.data());
        }

        webhooks.emplace_back(wh);
    }
    
    bot->log(dpp::ll_info, std::format("Loaded channel config ({} channels).", webhooks.size()));
}

void report::bot::loadWordConfig(const std::string configPath) {
    std::fstream stream{ configPath, stream.in };

    if (!stream.is_open())
        throw ConfigValueException("Couldn't load word config.");

    for (std::string line; std::getline(stream, line);) {
        if (line.empty() || line.at(0) == '#')
            continue;
        
        words.emplace_back(line);
    }
    
    bot->log(dpp::ll_info, std::format("Loaded word config ({} words).", words.size()));
}

void report::bot::loadTokenConfig(const std::string configPath) {
    std::fstream stream{ configPath, stream.in };

    if (!stream.is_open())
        throw ConfigValueException("Couldn't load token config.");

    for (std::string line; std::getline(stream, line);) {
        if (line.empty() || line.at(0) == '#')
            continue;
        
        botToken = line;
    }
    
//    bot->log(dpp::ll_info, std::format("Loaded word config ({} words).", words.size()));
}

void report::bot::handler() noexcept {
    try {
        loadTokenConfig("token.conf");
    }
    catch (ConfigValueException &e) {
        std::cout << e.what() << '\n';
        return;
    }
    
    dpp::cluster newBot(botToken.data(), dpp::i_default_intents | dpp::i_message_content);
    
    bot = &newBot;        
    bot->on_log(dpp::utility::cout_logger());

    try {
        report::bot::loadChannelConfig("channel.conf");
        report::bot::loadWordConfig("words.conf");
    }
    catch (ConfigValueException &e) {
        bot->log(dpp::ll_critical, e.what());
        return;
    }
    
    
	bot->on_message_create([](const dpp::message_create_t &event) {
		if (!event.msg.author.is_bot()) {
			handleMessage(event);
		}
	});
  
    bot->on_ready([](const dpp::ready_t &event) {
        (void)event;
        
        bot->set_presence(dpp::presence(dpp::ps_dnd, dpp::at_custom, "I'm watching you."));
        });

    bot->start(dpp::st_wait);
}
