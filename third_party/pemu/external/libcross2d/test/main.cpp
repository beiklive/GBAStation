//
// Created by cpasjuste on 08/12/16.
//

#include "cross2d/c2d.h"

using namespace c2d;

int main(int argc, char *argv[]) {
    // create main renderer
    auto renderer = new C2DRenderer();
    renderer->setPrintStats(true);
    renderer->setClearColor(Color::GrayLight);

    auto border = new C2DRectangle({2, 2, renderer->getSize().x - 4, renderer->getSize().y - 4});
    border->setFillColor(Color::Transparent);
    border->setOutlineColor(Color::Red);
    border->setOutlineThickness(2);
    renderer->add(border);

    // scale font and texture for different resolution (devices),
    // based on 720p resolution
    float scaling = renderer->getSize().y / 720.0f;

    // create a rect
    auto rect = new C2DRectangle(
            {renderer->getSize().x / 2, renderer->getSize().y / 2,
             renderer->getSize().x / 2, renderer->getSize().y / 2});
    rect->setOrigin(Origin::Center);
    rect->setFillColor(Color::Gray);
    rect->setOutlineColor(Color::Orange);
    rect->setOutlineThickness(8 * scaling);

    // create a texture and add it to the rect
    auto tex = new C2DTexture(renderer->getIo()->getRomFsPath() + "icon.png");
    if (tex->available) {
        tex->setPosition(rect->getSize().x / 2, rect->getSize().y / 2);
        tex->setScale(scaling, scaling);
        tex->setOrigin(Origin::Center);
        rect->add(tex);
    }

#ifndef __NO_FREETYPE__
    // create a text
    auto font = new Font();
    font->loadFromFile(renderer->getIo()->getRomFsPath() + "future.ttf");
    auto text = new Text("libcross2d @ Cpasjuste", 18, font);
    text->setFillColor(Color::Yellow);
    text->setOutlineThickness(2);
    text->setPosition(renderer->getSize().x - 8, renderer->getSize().y - 8);
    text->setOrigin(Origin::BottomRight);
    renderer->add(text);
#endif

    // create a bitmap text (https://www.angelcode.com/products/bmfont/)
    auto bmFont = new BMFont();
    bmFont->loadFromFile(renderer->getIo()->getRomFsPath() + "future.fnt");
    auto bmText = new Text("libcross2d @ Cpasjuste", 18, bmFont);
    bmText->setFillColor(Color::Red);
    bmText->setPosition(renderer->getSize().x - 8, renderer->getSize().y - 36);
    bmText->setOrigin(Origin::BottomRight);
    renderer->add(bmText);

    // add all this crap to the renderer
    renderer->add(rect);

    // add some tweening :)
    auto tweenPos = new TweenPosition(
            {renderer->getSize().x / 2 - (256 * scaling), rect->getPosition().y},
            {renderer->getSize().x / 2 + (256 * scaling), rect->getPosition().y},
            4.0f, TweenLoop::PingPong);
    tweenPos->play();
    rect->add(tweenPos);
    auto tweenRot = new TweenRotation(0, 360, 5.0f, TweenLoop::PingPong);
    tweenRot->play();
    rect->add(tweenRot);
    auto tweenScale = new TweenScale(rect->getScale(), {2.0f, 2.0f}, 3.0f, TweenLoop::PingPong);
    tweenScale->play();
    rect->add(tweenScale);
    auto tweenColor = new TweenColor(rect->getFillColor(), Color(0x9bc4e2ff), 3.0f, TweenLoop::PingPong);
    tweenColor->play();
    rect->add(tweenColor);
    auto tweenAlpha = new TweenAlpha(255, 200, 3.0f, TweenLoop::PingPong);
    tweenAlpha->play();
    rect->add(tweenAlpha);

    while (true) {

        // stop if any key is pressed
        if (renderer->getInput()->getButtons() != 0) {
            break;
        }

        // renderer everything
        renderer->flip();
    }

#ifndef __NO_FREETYPE__
    delete (font);
#endif
    delete (bmFont);

    // will delete child's (textures, shapes, text..)
    delete (renderer);

    return 0;
}