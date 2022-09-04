#define PAGES 3 // the length of the table

// DO NOT CHANGE THESE CONSTANTS -- they are purely for simplifying the table-making process
#define RANDOM 255
#define SWITCH_TIME 0b100
#define SWITCH_LIGHT 0b010
#define SWITCH_BUTTON 0b001

// page id [|] button position [|] next page (button) [|] next page (default) [|] switch mode (time | light | button)

// next page (button) indicates the page id to move to when the button is pressed
// next page (default) indicates the page id to move to when the page is to move, but the button has not been pressed

const byte table[][5] {
  { 0, 245, 2, RANDOM, SWITCH_BUTTON },
  { 1, 0,   0, 2,      SWITCH_BUTTON },
  { 2, 90,  1, RANDOM, SWITCH_BUTTON }
};
