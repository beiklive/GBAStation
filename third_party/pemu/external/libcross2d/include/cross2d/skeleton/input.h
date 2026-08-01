//
// Created by cpasjuste on 11/01/17.
//

#ifndef __C2D_INPUT_H__
#define __C2D_INPUT_H__

#include <string>
#include "cross2d/skeleton/sfml/Clock.hpp"
#include "cross2d/skeleton/sfml/Vector2.hpp"

#ifndef BIT
#define BIT(n) (1U<<(n))
#endif

#define PLAYER_MAX 8

namespace c2d {

    class Input {

    public:

        enum Rotation {
            R0,
            R90,
            R180,
            R270
        };

        enum Button : unsigned int {
            Up = BIT(1),
            Down = BIT(2),
            Left = BIT(3),
            Right = BIT(4),
            Select = BIT(5),
            Start = BIT(6),
            A = BIT(7),
            B = BIT(8),
            X = BIT(9),
            Y = BIT(10),
            LT = BIT(11),
            RT = BIT(12),
            LB = BIT(13),
            RB = BIT(14),
            LS = BIT(15),
            RS = BIT(16),
            Menu1 = BIT(17),
            Menu2 = BIT(18),
            Touch = BIT(19),
            Delay = BIT(20),
            Quit = BIT(21)
        };

        struct ButtonMapping {
            Button button;
            int value;
        };

        struct Axis {
            int id;
            short value;
        };

        struct Player {
            std::vector<ButtonMapping> mapping{};
            std::vector<ButtonMapping> mapping_default{};
            Axis lx{};
            Axis ly{};
            Axis rx{};
            Axis ry{};
            unsigned int buttons = 0;
            int dz = 8000;
            bool enabled = false;
            void *data = nullptr;
            int id = 0;
            // touch
            Vector2f touch;
        };

        // map keyboard to player 0
        struct Keyboard {
            std::vector<ButtonMapping> mapping;
            std::vector<ButtonMapping> mapping_default{};
        };

        explicit Input();

        virtual ~Input();

        virtual Player *update();

        virtual int waitButton(int player = 0) { return -1; };

        virtual unsigned int getButtons(int player = 0);

        virtual Player *getPlayer(int player = 0);

        virtual Player *getPlayers();

        virtual std::vector<ButtonMapping> getKeyboardMapping();

        virtual std::vector<ButtonMapping> getKeyboardMappingDefault();

        virtual int clear(int player = 0);

        virtual void setRepeatDelay(int ms);

        virtual int getRepeatDelay();

        virtual void setRotation(const Rotation &dirRotation, const Rotation &buttonRotation);

        virtual Rotation getDirRotation();

        virtual Rotation getButtonRotation();

        virtual void setJoystickMapping(int player, const std::vector<ButtonMapping> &mapping,
                                        const Vector2i &leftAxis, const Vector2i &rightAxis, int dz);

        virtual void setKeyboardMapping(const std::vector<ButtonMapping> &mapping);

    protected:
        virtual Vector2f getAxisState(const Player &player, int xAxis, int yAxis) { return {}; };

        virtual int getButtonState(const Player &player, int button) { return false; };

        virtual int getKeyState(int key) { return false; };

        virtual Vector2f getTouch() { return {}; };

        virtual void applyRotation(Player *player);

        Player m_players[PLAYER_MAX];
        Keyboard m_keyboard{};

    private:
        Clock *m_repeatClock;
        int m_repeatDelay = 150;
        bool m_repeat = false;
        unsigned int m_stateOld = 0;
        Rotation m_dir_rotation = R0;
        Rotation m_button_rotation = R0;
    };
}

#endif //__C2D_INPUT_H__
