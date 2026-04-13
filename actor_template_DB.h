#ifndef ACTOR_TEMPLATE_DB_H
#define ACTOR_TEMPLATE_DB_H
#include <algorithm>
#include <unordered_map>
#include "Helper.h"
#include "actor.h"
#include "scene_DB.h"

inline void ReadComponentAttributes(const rapidjson::Value& json, actor_component& component) {
    if (json.HasMember("type") && json["type"].IsString())
        component.type = json["type"].GetString();

    for (auto property_it = json.MemberBegin(); property_it != json.MemberEnd(); ++property_it) {
        const std::string property_name = property_it->name.GetString();
        if (property_name == "type")
            continue;

        const auto& property_value = property_it->value;
        if (property_value.IsBool())
            component.properties[property_name] = property_value.GetBool();
        else if (property_value.IsInt64())
            component.properties[property_name] = property_value.GetInt64();
        else if (property_value.IsNumber())
            component.properties[property_name] = property_value.GetDouble();
        else if (property_value.IsString())
            component.properties[property_name] = std::string(property_value.GetString());
    }
}

inline void ReadActorComponents(const rapidjson::Value& json, actor& a) {
    if (!json.HasMember("components") || !json["components"].IsObject())
        return;

    const auto& json_components = json["components"];
    for (auto it = json_components.MemberBegin(); it != json_components.MemberEnd(); ++it) {
        const std::string component_key = it->name.GetString();
        const auto& component_object = it->value;

        auto existing_component = std::find_if(a.components.begin(), a.components.end(),
                                               [&](const actor_component& component) {
                                                   return component.key == component_key;
                                               });

        if (existing_component != a.components.end()) {
            ReadComponentAttributes(component_object, *existing_component);
            continue;
        }

        actor_component component;
        component.key = component_key;
        ReadComponentAttributes(component_object, component);
        a.components.emplace_back(std::move(component));
    }

    std::sort(a.components.begin(), a.components.end(),
              [](const actor_component& left, const actor_component& right) {
                  return left.key < right.key;
              });
}

class actor_template{
public:
    std::unordered_map<std::string, actor> templates;
    void load_templates(){
        templates.clear();
        std::string dir = "resources/actor_templates";
        if (!std::filesystem::exists(dir)) return;
        for (const auto& entry : std::filesystem::directory_iterator("resources/actor_templates")) {
            if (!entry.is_regular_file()) continue;
            if (entry.path().extension() != ".template") continue;

            const std::string config_path_template = entry.path().string();
            const std::string template_name = entry.path().stem().string();

            rapidjson::Document json_template;
            Helper::ReadJsonFile(config_path_template, json_template);
            actor t;
            if (json_template.HasMember("name"))
                t.name = json_template["name"].GetString();
            ReadActorComponents(json_template, t);

            templates.insert_or_assign(template_name, std::move(t));
        }
    }
};
#endif // !ACTOR_TEMPLATE_DB_H
