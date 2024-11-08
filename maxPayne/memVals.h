#pragma once

// Class containing important default values for various params in memory.
class defaultVals {

private:
	float defaultGameSpeed;
	float defaultCamSpeed;

public:
	
	// Constructor.
	defaultVals() {
		defaultGameSpeed = 0.01666666754f;	// Obvious.
		defaultCamSpeed = 0.01666666754f;	// Relates to how fast the camera follows you.
	}

	// Getters (no need for setters as the values in this class are meant to be unchanged).
	float getDefaultGameSpeed() const {
		return defaultGameSpeed;
	}
	float getDefaultCamSpeed() const {
		return defaultCamSpeed;
	}
};

// Class containing values to be substituted for various params in memory.
class newVals {

private:
	float payneGameSpeed;
	float payneCamSpeed;

public:

	// Constructor.
	newVals() {
		payneGameSpeed = 0.008333333768f;	// For max payne bullet time, halves the speed of the game.
		payneCamSpeed = 0.01999999955f;		// For max payne bullet time, makes the camera slower to follow (also makes game speed slower for some reason).
	}

	// Getters (no need for setters as the values in this class are meant to be unchanged).
	float getPayneGameSpeed() const {
		return payneGameSpeed;
	}
	float getPayneCamSpeed() const {
		return payneCamSpeed;
	}
};