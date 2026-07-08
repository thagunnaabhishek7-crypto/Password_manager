#include <SFML/Graphics.hpp>
#include <iostream>
int main() {
    auto window = sf::RenderWindow(sf::VideoMode({800, 600}), "PS Manager");
    window.setFramerateLimit(60);

    sf::CircleShape shape(100.f);
    shape.setFillColor(sf::Color::Green);

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();
        }
        // std::cout<<"Running"<<std::endl;
        window.clear();
        window.draw(shape);
        window.display();
    }
}