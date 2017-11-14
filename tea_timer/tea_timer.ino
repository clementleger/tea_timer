#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <avr/sleep.h>

/**
 * Blinking interval when time is over
 */
#define BLINKING_TIME   500

#define TIME_BEFORE_SLEEP  (10 * 1000)

/**
 * Blinking interval when time is over
 */
#define BUZZER_PIN      5
#define SCREEN_PIN      6

#define BUTTON1_PIN     2
#define BUTTON2_PIN     3
#define BUTTON3_PIN     4

/**
 * Size of yellow band at top of OLED display
 */
#define HEADER_SIZE 16

Adafruit_SSD1306 display;

#if (SSD1306_LCDHEIGHT != 64)
#error "Height incorrect, please fix Adafruit_SSD1306.h!"
#endif

void wakeHandler()
{

}

void setup()
{
	pinMode(BUZZER_PIN, OUTPUT);
	pinMode(SCREEN_PIN, OUTPUT);
	pinMode(BUTTON1_PIN, INPUT_PULLUP);
	pinMode(BUTTON2_PIN, INPUT_PULLUP);
	pinMode(BUTTON3_PIN, INPUT_PULLUP);
 
	digitalWrite(SCREEN_PIN, HIGH);
	delay(200);
	Serial.begin(9600);

	// by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
	display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
}

static void displayIdle()
{
	display.clearDisplay();  
	display.setTextSize(2);
	display.setTextColor(WHITE);
	display.setCursor(10,0);
	display.println("Tea Timer");
	display.setCursor(35, HEADER_SIZE);
	display.println("Press");
	display.setCursor(10, (HEADER_SIZE + 1) * 2);
	display.println("button to");
	display.setCursor(35, (HEADER_SIZE + 1) * 3);
	display.println("start");
	display.display();
}

static void displayTime(unsigned long timeout)
{
	String t;
	display.fillRect(0, HEADER_SIZE, display.width(), display.height() - HEADER_SIZE, BLACK);
	display.setCursor(5,HEADER_SIZE + 4);

	t = String(timeout / 60);
	t = t + ":";
	if (timeout % 60 < 10)
		t = t + "0";
	t = t + String(timeout % 60);

	display.println(t);
	display.display();
}

/* Get pressed button 
 * No debouncing at all... */
static int buttonPress()
{
	if (digitalRead(BUTTON1_PIN) == LOW)
		return 1;
	if (digitalRead(BUTTON2_PIN) == LOW)
		return 2;
	if (digitalRead(BUTTON3_PIN) == LOW)
		return 3;

	return 0;
}

/* Run and display the timer */
static int runTimer(long timeout)
{
	display.setTextSize(5);
	unsigned long current_millis = 0;
	displayTime(timeout);
	delay(200);
	while (timeout >= 0) {
		if (buttonPress() != 0)
			return 1;
		if ((millis() - current_millis) < 1000)
			continue;
		current_millis = millis();
		displayTime(timeout);
		timeout--;
	}
	delay(1000);
	
	return 0;
}

/* Display end of timer signal and enable buzzer */
static void endOfTimer()
{
	unsigned long current_millis = millis();
	int blink_state = 1;

	/* End of timer, display blinking time and enable buzzer */
	display.clearDisplay();  
	display.setTextSize(2);
	display.setTextColor(WHITE);
	display.setCursor(10,0);
	display.println("Finish !");
	display.display();

	display.setTextSize(5);
	while (buttonPress() == 0) {
		if ((millis() - current_millis) < 500)
			continue;
		
		current_millis = millis();

		if (blink_state == 0) {
			display.fillRect(0, HEADER_SIZE, display.width(), display.height() - HEADER_SIZE, BLACK);
			display.display();
			noTone(BUZZER_PIN);
		} else {
			displayTime(0);
			tone(BUZZER_PIN, 1000);
		}

		blink_state = blink_state ? 0 : 1;
	}
	noTone(BUZZER_PIN);
}

static void sleepTimer()
{
	/* Prepare for sleeping */
	delay(100);
	digitalWrite(SCREEN_PIN, LOW);
	attachInterrupt(digitalPinToInterrupt(BUTTON1_PIN), wakeHandler, LOW); 
	attachInterrupt(digitalPinToInterrupt(BUTTON2_PIN), wakeHandler, LOW);

	/* zzzzzzzzzZZZZZZZZZZzzzzzzzzzzzzz */
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	sleep_enable();
	sleep_mode();

	/* Wake up from sleep */
	sleep_disable();
	detachInterrupt(digitalPinToInterrupt(BUTTON1_PIN));
	detachInterrupt(digitalPinToInterrupt(BUTTON2_PIN));

	/* Wake up screen ! */
	digitalWrite(SCREEN_PIN, HIGH);
	delay(200);
	display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
	displayIdle();
}

void loop()
{
  unsigned long start_millis;
	long timeout = 2 * 60;
	int button;
	int ret;
 
	displayIdle();
	delay(400);
	start_millis = millis();
	do {
		button = buttonPress();
		/* Put micro in sleep if idle for too much time */
		if ((millis() - start_millis) >= TIME_BEFORE_SLEEP) {
			sleepTimer();
			start_millis = millis();
		}
	} while(button == 0);

	/* Start the timer according to the rpessed button */
	ret = runTimer(timeout + button * 60);
	if (ret)
		return;

	endOfTimer();
}

