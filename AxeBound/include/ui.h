//ui.h
//UI Classes

// Games Engineer Group Project - Set09121

// Group H
// 10-2026



//Known Bugs In Program

// maybe add visble checks? so the buttons dont do anything is there not viible?

#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp> // temp < why?
#include <functional>
#include <algorithm>
#include <memory>
#include <vector>
#include <string>
//#include "logger.h"
#include "globals.h"

class uiElement {
	protected:
		sf::Vector2f position = {0.0f, 0.0f}; // ?top left of element? // pos on 720p display
		sf::Vector2f size = {0.0f, 0.0f}; // size on a 720p display
		float elementScale = 1.0f;
		float elementScaleLastFrame = -1.0f;
		sf::Vector2u screenSize = {0, 0};
		bool visible = true; // CHANGED: visible by default (it was hidden by default and nothing ever showed it)
		
		void updateElementScale(sf::Vector2u newScreenSize) { // scale is based on window size
			if (newScreenSize == screenSize) { // screen hasnt changed why have i been called? (saves on maths if this is called for some reason when it shouldnt, for example in a loop)
				//gameLog.violation("UI updateElementScale Method Called when window has not changed size. This Should Not Have Been Called!");
				return;
			}
			
			float scaleX = static_cast<float>(newScreenSize.x) / 1280.0f ;
			float scaleY = static_cast<float>(newScreenSize.y) / 720.0f ;
			elementScale = std::min(scaleX, scaleY);
			screenSize = newScreenSize; // update it now as the scale has been changed
		}
	public:
		uiElement() = default;
		
		explicit uiElement(sf::Vector2u newWindowSize, sf::Vector2f elementPos, sf::Vector2f elementSize){
			position = elementPos;
			size = elementSize;
			screenSize = {0, 0};
			visible = true;
			elementScale = 1;
			elementScaleLastFrame = -1.0f;// CHANGED: -1 so the elements always lay themselves out on the first update
			updateElementScale(newWindowSize);
		}
		/*void setPosition(float x, float y){
			
			//position = {x,y};
		}
		void setSize(float x, float y){ // depreciated
			//size = {x,y};
		}*/
		void setVisible(bool val){
			visible = val;
		}
		bool isVisible(){
			return visible;
		}
		virtual void updateWindowSize(sf::Vector2u newWindowSize) { // call on window resize
			updateElementScale(newWindowSize);
		}
		
		virtual void update(float dt, float x = 0.0f, float y = 0.0f, bool clicked = false) = 0;
		virtual void draw(sf::RenderWindow &window) {
			if (!visible) {return;}// add log error.
		}
		
		virtual ~uiElement() = default;
};

// basic uiElements
class label : public uiElement {
	private:
		sf::Text elementText; // = nullptr?
		sf::RectangleShape box; // CHANGED: optional coloured background (the green banners/panels in the mock-ups)
		bool hasBox = false, alignLeft = false, dirty = true;
		float textPx = 0.0f; // fixed text height on the 720p canvas (0 = half the box height)
		void layoutLabel(){
			float s = elementScale;
			if (hasBox){
				box.setPosition(position.x*s, position.y*s);
				box.setSize(sf::Vector2f(size.x*s, size.y*s));
				box.setOutlineThickness(2.0f*s);
				float px = (textPx > 0.0f) ? textPx : size.y*0.5f;
				elementText.setCharacterSize(static_cast<unsigned>(std::max(1.0f, px*s)));
				elementText.setScale(1.0f, 1.0f);
				sf::FloatRect b = elementText.getLocalBounds();
				if (alignLeft){
					elementText.setOrigin(0.0f, 0.0f);
					elementText.setPosition(position.x*s + 10.0f*s, position.y*s + 8.0f*s);
				}else{
					float maxW = size.x*s*0.95f;
					if (b.width > maxW && b.width > 0.0f){ float f = maxW/b.width; elementText.setScale(f, f); }
					elementText.setOrigin(b.left + b.width/2.0f, b.top + b.height/2.0f);
					elementText.setPosition((position.x + size.x/2.0f)*s, (position.y + size.y/2.0f)*s);
				}
			}else{
				elementText.setPosition(position.x*s,position.y*s);
				elementText.setCharacterSize(static_cast<int>(size.y*s));
			}
			elementScaleLastFrame = s;
			dirty = false;
		}
	public:
		label() = default;
		
		label(sf::Vector2u newWindowSize, sf::Vector2f elementPos, sf::Vector2f elementSize, std::string labelText, sf::Font& gameFont, sf::Color colour) : 
		uiElement(newWindowSize, elementPos, elementSize),
		elementText(labelText, gameFont){
			elementText.setFillColor(colour);
		}
		void setBackground(sf::Color fill, sf::Color outline){ hasBox = true; box.setFillColor(fill); box.setOutlineColor(outline); dirty = true; }
		void setAlignLeft(bool a){ alignLeft = a; dirty = true; }
		void setTextSize(float px){ textPx = px; dirty = true; }
		void update(float dt, float x = 0.0f, float y = 0.0f, bool clicked = false) override { // add pointer to the input list
			if (!visible) return;
			if (elementScale != elementScaleLastFrame || dirty) layoutLabel();
		}
		void draw(sf::RenderWindow &window) override {
			if (!visible) return;
			if (hasBox) window.draw(box);
			window.draw(elementText);
		}
		void updateText(std::string newText) {
			elementText.setString(newText);
			dirty = true;
		}
};
class image : public uiElement {
	private:
		sf::Sprite imageSprite; // = nullptr?
		sf::Vector2f textureSize = {1.0f, 1.0f};
	public:
		image() = default;
		
		image(sf::Vector2u newWindowSize, sf::Vector2f elementPos, sf::Vector2f elementSize, sf::Texture& texture) : 
		uiElement(newWindowSize, elementPos, elementSize),
		imageSprite(texture){ // maybe change texture to an atlas?
			sf::Vector2u t = texture.getSize();
			textureSize = sf::Vector2f(static_cast<float>(std::max(1u, t.x)), static_cast<float>(std::max(1u, t.y)));
		}
		void update(float dt, float x = 0.0f, float y = 0.0f, bool clicked = false) override { // add pointer to the input list
			if (elementScale != elementScaleLastFrame){
				//pos update
				imageSprite.setPosition(position.x*elementScale,position.y*elementScale);
				//size update
				imageSprite.setScale(size.x*elementScale/textureSize.x,size.y*elementScale/textureSize.y); // CHANGED: size / texture size
				//store last scale
				elementScaleLastFrame = elementScale;
			}
		}
		void draw(sf::RenderWindow &window) override {
			if (!visible) return;
			window.draw(imageSprite);
		}
};

class interactableElement : public uiElement {
	protected:
		sf::Text elementText; // = nullptr?
		sf::Font font;
		sf::Texture texture;
		sf::RectangleShape outlineRectangle;
		sf::RectangleShape mainRectangle;
		bool pressed = false; // maybe only needed for textbox?
		float outlinePx = 2.0f; // CHANGED: outline thickness in 720p pixels (the old outLineScale made wide buttons get a huge outline)
		bool dirty = true;      // CHANGED: set when the text changes so the layout is redone
		bool enabled = true;    // CHANGED: disabled elements are greyed out and ignore the mouse
		bool selected = false;  // CHANGED: highlighted save slot / difficulty / tool
		float lastAppliedScale = -1.0f;
		sf::Color baseColour = sf::Color(0x92, 0xd0, 0x50);
		sf::Color hoverColour = sf::Color(0x56, 0xbd, 0x3b);
		sf::Color textColourNormal = sf::Color::Black;
		//element scale for ui>window scaling
		
		//methods
		bool isInRange(float value, float lower, float upper){ // inclusive? or exclusive?
			if (value > lower && value < upper) {return true;}
			return false;
		}
		bool isMouseAboveMe(float x, float y){ // CHANGED: works (it only failed because nothing was visible / sized)
			bool returnValue = true; // assume mouse is over button
			if (!isInRange(x, outlineRectangle.getPosition().x, outlineRectangle.getPosition().x+outlineRectangle.getSize().x)) {returnValue = false;} // if the mouse isnt within the X coordinates, its not over the button
			if (!isInRange(y, outlineRectangle.getPosition().y, outlineRectangle.getPosition().y+outlineRectangle.getSize().y)) {returnValue = false;} // if the mouse isnt within the Y coordinates, its not over the button
			return returnValue;
		}
		void updateScales(bool otherCondition, float currentScale){
			if (elementScale != elementScaleLastFrame || otherCondition || currentScale != lastAppliedScale || dirty)
			{
				float baseWidth = size.x * elementScale;
				float baseHeight = size.y * elementScale; // for center calc

				// combined scale for elemnt
				float combinedScale = elementScale * currentScale;

				float width = size.x * combinedScale;
				float height = size.y * combinedScale;
				// center
				float centerX = position.x * elementScale + baseWidth / 2.0f;
				float centerY = position.y * elementScale + baseHeight / 2.0f;
				//  outline thickness
				float outlineThickness = outlinePx * 2.0f * elementScale; // CHANGED: fixed pixel thickness
				float outlineWidth = width + outlineThickness; // outline shape size (width/height of all elements + thicknes)
				float outlineHeight = height + outlineThickness;

				// repos button
				mainRectangle.setPosition(centerX - width / 2.0f, centerY - height / 2.0f);
				outlineRectangle.setPosition(centerX - outlineWidth / 2.0f, centerY - outlineHeight / 2.0f);

				//text
				elementText.setPosition(centerX,centerY);
				elementText.setScale(1.0f, 1.0f);
				// centre  the texts origin
				sf::FloatRect textBounds = elementText.getLocalBounds();
				elementText.setOrigin(textBounds.left + textBounds.width / 2.0f, textBounds.top + textBounds.height / 2.0f);
				// CHANGED: instead of a fixed x2 the text is scaled to fit the button (better way to make font bigger? yes)
				float fit = 1.0f;
				if (textBounds.width > 0.0f && textBounds.height > 0.0f) fit = std::min(width * 0.9f / textBounds.width, height * 0.6f / textBounds.height);
				elementText.setScale(fit, fit);

				// resize button
				mainRectangle.setSize({width, height});
				outlineRectangle.setSize({outlineWidth, outlineHeight});

				elementScaleLastFrame = elementScale;
				lastAppliedScale = currentScale;
				dirty = false;
			}
		}
		void refreshColours(bool hovered){
			if (hovered && enabled) mainRectangle.setFillColor(hoverColour);
			else if (selected) mainRectangle.setFillColor(hoverColour);
			else mainRectangle.setFillColor(baseColour);
			elementText.setFillColor(enabled ? textColourNormal : sf::Color(0x7f, 0x7f, 0x7f));
		}
	public:
		explicit interactableElement(sf::Vector2u newWindowSize, sf::Vector2f elementPos, sf::Vector2f elementSize,
        sf::Font& gameFont, sf::Color textColour, sf::Color outlineColour, sf::Color elementColour, std::string labelText)  // new inputs
		:uiElement(newWindowSize, elementPos, elementSize) /* init parent*/,
		elementText(labelText, gameFont) { // text init
			font = gameFont;
			outlineRectangle = sf::RectangleShape();
			mainRectangle = sf::RectangleShape();
			outlineRectangle.setFillColor(outlineColour);
			mainRectangle.setFillColor(elementColour);
			elementText.setFillColor(textColour);
			baseColour = elementColour;
			textColourNormal = textColour;
			updateElementScale(newWindowSize);
			// (the old "FIX" note is resolved: updateScales() does all the layout)
		}
		
		virtual void update(float dt, float x, float y, bool clicked) {}
		
		// CHANGED: small helpers used by the menus / HUD
		void setText(const std::string& newText){ elementText.setString(newText); dirty = true; }
		void setEnabled(bool e){ enabled = e; refreshColours(false); }
		bool isEnabled() const { return enabled; }
		void setSelected(bool s){ selected = s; refreshColours(false); }
		void setColours(sf::Color base, sf::Color hover){ baseColour = base; hoverColour = hover; refreshColours(false); }
		
		void draw(sf::RenderWindow &window) override { // same across both?
			if (!visible) {return;} // CHANGED: hidden elements are not drawn
			window.draw(outlineRectangle);
			window.draw(mainRectangle);
			window.draw(elementText);
		}
		void updateWindowSize(sf::Vector2u newWindowSize) override {
			uiElement::updateWindowSize(newWindowSize);
			//updateElementShapes();
		}
		virtual ~interactableElement() {}
};

class uiButton : public interactableElement { // CHANGED: no longer broken
	private:
		//std::string buttonText;
		std::function<void()> action; // change this to a call back
		// button hovering
		float defaultScale = 1.0f;
		float currentScale = 1.0f;
		float scalePerSecond = 0.25f; // ?could be a constant?
		float maxScale = 1.05f; // ?could also be, no?
		
		void clickEvent(){ // overlapping buttons could cause issues? so dont over lap buttons!
			// if {x,y} within size area at position, this has been pressed, then do action
			// use a callback
			if (action) {action();}
		}
	public:
		// init font in constructor, also init text, and get text
		uiButton(sf::Vector2u newWindowSize, sf::Vector2f elementPos, sf::Vector2f elementSize, // class A properties
        sf::Font& gameFont, sf::Color textColour, sf::Color outlineColour, sf::Color elementColour, std::string labelText,  // CLASS B Propertires
		std::function<void()> callBackFunction) // this class (C) properties
		: interactableElement(newWindowSize, elementPos, elementSize, gameFont, textColour, outlineColour, elementColour, labelText){ 
			action = callBackFunction;
		}
		void setAction(std::function<void()> callBackFunction){ action = callBackFunction; }
		
		void update(float dt, float x, float y, bool clicked) override {
			if (!visible) return;
			bool hovered = enabled && isMouseAboveMe(x,y);
			if (hovered){
				// CHANGED: clamped so the scale lands exactly on max/default (it used to overshoot and stay slightly off forever)
				currentScale = std::min(maxScale, currentScale + scalePerSecond*dt);
			}else{
				currentScale = std::max(defaultScale, currentScale - scalePerSecond*dt);
			}
			refreshColours(hovered);
			updateScales(false, currentScale); // the duplicated layout code that used to follow was removed, updateScales() does it
			if (hovered && clicked) {clickEvent();}
		}
		
		virtual ~uiButton() {}
};


class inputTextBox : public interactableElement {
	private:
		std::string placeholderText;
		std::string inputedText;
		bool textUpdated = false;
		sf::Color placeHolderColour = sf::Color::White;
		int queueLength = 5; // default probably dont need to be declared!
		
	public:
		// init font in constructor, also init text, and get text
		inputTextBox(sf::Vector2u newWindowSize, sf::Vector2f elementPos, sf::Vector2f elementSize, // class A properties
        sf::Font& gameFont, sf::Color textColour, sf::Color outlineColour, sf::Color elementColour, std::string labelText,  // CLASS B Propertires
		int maxTextToDisplay)// class c (this ones!)
		: interactableElement(newWindowSize, elementPos, elementSize, gameFont, textColour, outlineColour, elementColour, labelText){ 
			//updateElementScale(newWindowSize);
			//inputedText="  ";
			placeholderText = labelText;
			elementText.setFillColor(textColour);
			placeHolderColour = textColour;
			queueLength = maxTextToDisplay;
		}
		
		void update(float dt, float x, float y, bool clicked) override {         // ADD a display queue/buffer so no overflow (done below)
			if (!visible) return;
			bool above = isMouseAboveMe(x,y);
			if (above && clicked && !pressed) { // input selected
				pressed = true;
				elementText.setFillColor(sf::Color::Black);
				textUpdated = true;
			}
			if (!above && clicked && pressed) { // clicked somewhere else - deselect (works now)
				pressed = false;
				if ("" == inputedText) elementText.setString(placeholderText); // return text to placeholder text if nothing entered
				textUpdated = true;
			}
			if (pressed) {
				std::string displayQueue = inputedText;
				if (static_cast<int>(displayQueue.length()) > queueLength) displayQueue = displayQueue.substr(displayQueue.length() - queueLength);
				if (displayQueue.empty()) displayQueue = " "; // so the text always has a size to centre
				elementText.setString(displayQueue);
			}
			updateScales(textUpdated, 1.0f);
			textUpdated = false; // CHANGED: it was never reset so the layout was redone every frame
		}
		
		void inputKey(sf::Uint32 unicode){
			if (!pressed) return; // CHANGED: only type when the box is selected (it used to type while playing)
			if (unicode == 8) // bspace
			{
				if (!inputedText.empty()) {inputedText.pop_back();}
			}else if (unicode >= 32 && unicode < 127 && inputedText.length() < 20) {
				inputedText += static_cast<char>(unicode);
			}
			textUpdated = true;
		}
		
		std::string getInput(){
			return inputedText;
		}
		
		virtual ~inputTextBox() {}
};

// ADDED: a container so a whole menu page / HUD panel can be shown, hidden, updated, drawn and resized together
class uiGroup {
	public:
		std::vector<std::shared_ptr<uiElement>> elements;
		bool visible = true;
		template <class T>
		std::shared_ptr<T> add(std::shared_ptr<T> e){ elements.push_back(e); return e; }
		void setVisible(bool v){ visible = v; }
		void update(float dt, float x, float y, bool clicked){
			if (!visible) return;
			for (auto &e : elements) e->update(dt, x, y, clicked);
		}
		void draw(sf::RenderWindow &window){
			if (!visible) return;
			for (auto &e : elements) e->draw(window);
		}
		void resize(sf::Vector2u newWindowSize){
			for (auto &e : elements) e->updateWindowSize(newWindowSize);
		}
};
