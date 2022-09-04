// motor constants
#define REVERSE_1_PIN 8
#define REVERSE_2_PIN 9
#define ENABLE_1_PIN 5
#define ENABLE_2_PIN 4

#define ASSIST_ON_MS 500
#define ASSIST_OFF_MS 1000

// non-motor I/O
#define LED_PIN 3
#define PHOTO_PIN 10
#define SEED_PIN 1 // for RNG
#define BUTTON_SERVO_PIN 2
#define BUTTON_PIN 0

#define MIN_BUTTON_SERVO_VAL 61; // far left
#define MAX_BUTTON_SERVO_VAL 177; // far right

// metadata about the current setup
#define MAX_TUNE_ITTERATIONS 5
//#define PAGES 4 // relocated to table.h

// timeouts
#define INTERSWITCH_WAIT_TIMEOUT_MS 5000 // amout of time to wait before switching the page
#define INTERTUNE_WAIT_TIMEOUT_MS 400
#define LONG_NEXT_PAGE_TIMEOUT_MS 4000
#define SHORT_NEXT_PAGE_TIMEOUT_MS 1000
#define STALL_CYCLE_TIMEOUT_MS 3000;

// defining what is considered new page
#define NEW_PAGE_LIGHT true

#if !NEW_PAGE_LIGHT
#define LOW_LIGHT_THRESHOLD 2400
#define HIGH_LIGHT_THRESHOLD 3200
#else
#define LOW_LIGHT_THRESHOLD 3550
#define HIGH_LIGHT_THRESHOLD 3850
#define NIGHT_LIGHT_THRESHOLD 100
#define DAY_LIGHT_THRESHOLD 300 // useless value currently
#endif

// currently useless values about specific geometry of the moving button mechanism
#define MAJOR_LENGTH 4.75 // length of arm connected directly to the servo
#define MINOR_LENGTH 5.75 // length of the arm connected directly to the button slider
#define Y_OFFSET 4.25 // Y offset of the servo from the button
#define X_OFFSET 0.25 // X offset of the servo from the button (when button at far right)

// bounds for where the servo can rotate to prevent disaster
#define MIN_BUTTON_ROT 60
#define MAX_BUTTON_ROT 175
