//
// Created by cpasjuste on 23/02/2021.
//

#include <algorithm>
#include "ss_dreamcast.h"

using namespace ss_api;

static std::vector<Dreamcast::HeaderFix> headerFixList;

Dreamcast::HeaderFix Dreamcast::getHeaderFix(const std::string &headerName) {

    if (headerFixList.empty()) {
        headerFixList = {
                {"VIRTUA STRIKER 2 VER.2000.1",                              "Virtua Striker 2"},
                {"HEADHUNTER - SEGA - AMUZE - 2001",                         "Headhunter"},
                {"SHENMUE2 EU",                                              "Shenmue 2"},
                {"STREET FIGHTER3 DOUBLE IMPACT",                            "Street Fighter 3 - Double Impact"},
                {"SUPER PUZZLE FIGHTER2X FOR MATCHING SERVICE",              "Super Puzzle Fighter 2 X"},
                {"AERO DANCING TODOROKI TAICHOU NO HIMITSU DISK",            "Aero Dancing - Todoroki Taichou No Himitsu Disc"},
                {"ESPN INTERNATIONAL TRACK AND FIELD",                       "Ganbare Nippon! Olympic 2000"},
                {"POWER STONE 2 USA",                                        "Power Stone 2"},
                {"POWER STONE 2 PAL",                                        "Power Stone 2"},
                {"SKIES OF ARCADIA DISC 1",                                  "Skies of Arcadia"},
                {"SKIES OF ARCADIA DISC 2",                                  "Skies of Arcadia"},
                {"SKIES OF ARCADIA DISC1",                                   "Skies of Arcadia"},
                {"SKIES OF ARCADIA DISC2",                                   "Skies of Arcadia"},
                {"ROADSTERS RC7",                                            "Roadsters"},
                {"RESIDENT EVIL3",                                           "Resident Evil 3 - Nemesis"},
                {"ROYAL RUMBLE",                                             "Wwf Royal Rumble"},
                {"VF3TB",                                                    "Virtua Fighter 3tb"},
                {"STREET FIGHTER ALPHA3 SAIKYO-DOJO",                        "Street Fighter Alpha 3"},
                {"STREET FIGHTER3 DOUBLE IMPACT",                            "Street Fighter 3 - Double Impact"},
                {"ESL",                                                      "European Super League"},
                {"PSO VER.2",                                                "Phantasy Star Online Ver.2"},
                {"PSO",                                                      "Phantasy Star Online"},
                {"NFL QUARTERBACK CLUB 2001",                                "Nfl Qb Club 2001"},
                {"GRANDIA2",                                                 "Grandia 2"},
                {"CART RACING",                                              "Super Speed Racing"},
                {"WETRIXPLUS",                                               "Wetrix+"},
                {"ROGUESPEAR",                                               "Tom Clancy's Rainbow Six Rogue Spear"},
                {"MTV SPORTS SKATEBOARDING",                                 "Mtv Sports - Skateboarding Featuring Andy Mcdonald"},
                {"ARABIAN NIGHTS - PRINCE OF PERSIA",                        "Prince Of Persia - Arabian Nights"},
                {"VIGILANTE 8 SECOND OFFENSE",                               "Vigilante 8 - Second Offense"},
                {"RAILROAD TYCOON II DREAMCAST EDITION",                     "Railroad Tycoon II"},
                {"DEMOLITION RACER NO EXIT DC",                              "Demolition Racer - No Exit"},
                {"I - SPY",                                                  "Espion-age-nts"},
                {"BUZZ 2000",                                                "Disney/pixar's Buzz Lightyear Of Star Command"},
                {"SOUTH PARK CHEFS",                                         "South Park - Chef's Luv Shack"},
                {"DISNEY S DONALD DUCK GOIN QUACKER",                        "Disney's Donald Duck Quack Attack"},
                {"SIERRA SPORTS MAXIMUM POOL",                               "Maximum Pool"},
                {"DRAGONRIDERS - THE CHRONICLES OF PERN",                    "Dragon Riders - Chronicles Of Pern"},
                {"STAR WARS EPISODE ONE JEDI POWER BATTLES",                 "Star Wars - Episode I Jedi Power Battles"},
                {"STAR WARS EPISODE 1 RACER",                                "Star Wars - Episode I Racer"},
                {"STAR WARS: EPISODE 1 RACER",                               "Star Wars - Episode I Racer"},
                {"SPECOPS OMEGA SQUAD",                                      "Spec Ops 2 - Omega Squad"},
                {"SPEC OPS2: OMEGA SQUAD",                                   "Spec Ops 2 - Omega Squad"},
                {"FORMULA 1 WORLD GRAND PRIX FOR DREAMCAST",                 "F1 World Grand Prix for Dreamcast"},
                {"EVILTWIN",                                                 "Evil Twin - Cyprien's Chronicles"},
                {"ALONE IN THE DARK 4 - THE NEW NIGHTMARE",                  "Alone In The Dark - The New Nightmare"},
                {"MONACO GRAND PRIX RACING SIMULATION 2",                    "Monaco Grand Prix - Racing Simulation 2"},
                {"EOS",                                                      "Exhibition Of Speed"},
                {"WWTBAM",                                                   "Who Wants to Be a Millionaire"},
                {"SYDNEY 2000 THE OFFICIAL VIDEO GAME OF THE OLYMPIC GAMES", "Sydney 2000"},
                {"TAXI2",                                                    "Taxi 2 - Le Jeu"},
                {"MONACO GRAND PRIX RACING SIMULATION 2 ON-LINE",            "Monaco Grand Prix - Racing Simulation 2"},
                {"SWWS 2000 EE",                                             "Sega Worldwide Soccer 2000 - Euro Edition"},
                {"THE KING OF FIGHTERS DREAM MATCH 1999",                    "King of Fighters, The - Dream Match 1999"},
                {"THE NEXT TETRIS ONLINE EDITION",                           "The Next Tetris - On-line Edition"},
                {"AERODANCING F/TODOROKI TSUBASA NO HATSU-HIKO",             "Aero Dancing F - Todoroki Tsubasa no Hatsu Hikou"},
                {"First of the North Star",                                  "Fist Of The North Star"},
                {"NeoGeo Battle Coliseum",                                   "Neo-geo Battle Coliseum"},
                {"HELLO KITTY WAKUWAKU COOKIES",                             "Hello Kitty No Waku Waku Cookies"},
                {"NET GOLF",                                                 "Nettou Golf"},
        };
    }

    auto it = std::find_if(headerFixList.begin(), headerFixList.end(), [headerName](const HeaderFix &headerFix) {
        return headerFix.headerName == headerName;
    });

    if (it != headerFixList.end()) {
        return *it;
    }

    return {};
}
