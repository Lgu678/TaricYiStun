#include "../plugin_sdk/plugin_sdk.hpp"
#include "taric.h"
#include <chrono>

using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::system_clock;

namespace taric
{
    TreeTab* main_tab = nullptr;
    script_spell* e = nullptr;
    script_spell* r = nullptr;
    bool alphaStarted = false;
    bool awaitingEFire = false;
    int timeAlphaStarted = 0;

    namespace ui
    {
        TreeEntry* toggle_stun_keybind = nullptr;
        TreeEntry* toggle_ult_keybind = nullptr;
    }

    void on_draw();
    void on_process_spell_cast(game_object_script sender, spell_instance_script spell);
    void on_update();

    void setupMenu()
    {
        main_tab = menu->create_tab("MluAIO", "MluAIO Taric");
        main_tab->set_assigned_texture(myhero->get_square_icon_portrait());
        {
            main_tab->add_separator(myhero->get_model(), "MluAIO: ");
            ui::toggle_stun_keybind = main_tab->add_checkbox(myhero->get_model() + ".stun", "Use E before final Alphastrike Animation finishes", true);
            ui::toggle_ult_keybind = main_tab->add_checkbox(myhero->get_model() + ".ult.", "Sync Taric's Ult with Master Yi's Ult for instant engage", true);
        }
    }

    void load()
    {
        e = plugin_sdk->register_spell(spellslot::e, 1300);
        r = plugin_sdk->register_spell(spellslot::r, 0);
        setupMenu();
        event_handler<events::on_update>::add_callback(on_update);
        event_handler<events::on_draw>::add_callback(on_draw);
        event_handler<events::on_process_spell_cast>::add_callback(on_process_spell_cast);
    }

    void unload()
    {
        menu->delete_tab(main_tab);
        plugin_sdk->remove_spell(e);
        event_handler<events::on_update>::remove_handler(on_update);
        event_handler<events::on_draw>::remove_handler(on_draw);
        event_handler<events::on_process_spell_cast>::remove_handler(on_process_spell_cast);
    }

    void on_process_spell_cast(game_object_script sender, spell_instance_script spell)
    {
        if (ui::toggle_stun_keybind->get_bool() && sender && sender->get_champion() == champion_id::MasterYi && sender->get_team() == myhero->get_team() && spell->get_spellslot() == spellslot::q && !awaitingEFire && !alphaStarted)
        {
            if (!sender->has_buff(buff_hash("taricwallybuff")) || !sender->has_buff(buff_hash("taricwleashactive")) || !e->is_ready() || orbwalker->flee_mode())
                return;
            alphaStarted = true;
            timeAlphaStarted = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        }

        if (ui::toggle_ult_keybind->get_bool() && sender && sender->get_champion() == champion_id::MasterYi && sender->get_team() == myhero->get_team() && spell->get_spellslot() == spellslot::r)
        {
            if (!sender->has_buff(buff_hash("taricwallybuff")) || !sender->has_buff(buff_hash("taricwleashactive")) || !r->is_ready())
                return;

            r->cast();
        }
    }

    game_object_script get_closest_target(float range, vector from)
    {
        auto targets = entitylist->get_enemy_heroes();

        targets.erase(std::remove_if(targets.begin(), targets.end(), [range, from](game_object_script x)
            {
                return !x->is_valid_target(range, from) || x->is_dead();
            }), targets.end());

        std::sort(targets.begin(), targets.end(), [from](game_object_script a, game_object_script b)
            {
                return a->get_distance(from) < b->get_distance(from);
            });

        if (!targets.empty())
            return targets.front();

        return nullptr;
    }

    game_object_script getMasterYi()
    {
        std::vector<game_object_script> allies = entitylist->get_ally_heroes();

        for (int i = 0; i < allies.size(); i++)
        {
            if (allies[i]->get_champion() == champion_id::MasterYi)
            {
                return allies[i];
            }
        }
    }

    void fireTaricE()
    {
        game_object_script target = get_closest_target(575, getMasterYi()->get_position());

        if (target != nullptr) {
            myhero->cast_spell(spellslot::e, target, true, false);
        }

        awaitingEFire = false;
    }

    void on_update()
    {
        if (myhero->is_dead())
        {
            alphaStarted = false;
            awaitingEFire = false;
            return;
        }

        int time = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

        if (alphaStarted && time - timeAlphaStarted >= 300)
        {
            alphaStarted = false;
            awaitingEFire = true;
        }

        if (awaitingEFire && !getMasterYi()->has_buff(buff_hash("AlphaStrike")))
        {
            fireTaricE();
        }
    }

    void on_draw()
    {
        if (myhero->is_dead())
            return;
    }
}