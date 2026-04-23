#include <LiquidCrystal.h>

// Select the pins used on the LCD panel
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// Define some values used by the panel and buttons
int lcdKey = 0;
int adc_key_in = 0;

const int Write = A1;  // ปุ่มเพิ่มเติมที่ 1 เชื่อมต่อกับ Pin A1
const int Black = A2;  // ปุ่มเพิ่มเติมที่ 2 เชื่อมต่อกับ Pin A2

enum Button
{
    btnRIGHT,
    btnUP,
    btnDOWN,
    btnLEFT,
    btnSELECT,
    btnNONE,
};

// Read the buttons
int read_LCD_buttons()
{
    adc_key_in = analogRead(0);      // Read the value from the sensor

    // Check the main buttons
    if (adc_key_in > 1000) return btnNONE;
    if (adc_key_in < 50)   return btnRIGHT;
    if (adc_key_in < 250)  return btnUP;
    if (adc_key_in < 450)  return btnDOWN;
    if (adc_key_in < 650)  return btnLEFT;
    if (adc_key_in < 850)  return btnSELECT;

    return btnNONE;  // When all others fail, return this...
}

// Initialise some global parameters
bool isUsingMenu = false;  // Whether we are in the time-setting menu
bool isGameOver = false;

class Player
{
    // Represents a player.
public:
    int minutes;              // Number of minutes allowed
    String menuText;          // Text for the time-setting menu
    String playIndicator;     // Indicator for when active
    long secondsRun;          // Seconds run while the player was active
    
    void IncrementMinutes()
    {
        minutes = min(minutes + 1, 99);
        delay(250);
    }
    void DecrementMinutes()
    {
        minutes = max(minutes - 1, 1);
        delay(250);
    }
    int SecondsRemaining()
    {
        // Number of seconds remaining on the player's clock
        return minutes * 60 - secondsRun;
    }
    void UpdateCounter(Player p0, Player p1, Player p2)
    {
        secondsRun += millis() / 1000 - (p0.secondsRun + p1.secondsRun + p2.secondsRun);
    }
};

// Initialise the players
Player p0 = {0, "", "  --  ", 0};  // Dummy player, active when player clocks are not counting down
Player p1 = {5, "Player1 mins: ", "  <-  ", 0};
Player p2 = {5, "Player2 mins: ", "  ->  ", 0};

// Set a pointer to the first player to be shown in the time-setting menu
Player* menuPlayer = &p1;
// Set the active player pointer to the player to start with
Player* activePlayer = &p0;

void setup()
{
    lcd.begin(16, 2);
    pinMode(Write, INPUT);
    pinMode(Black, INPUT);
}

// Main loop
void loop()
{
    activePlayer->UpdateCounter(p0, p1, p2);  // Do this every loop
    lcdKey = read_LCD_buttons();

    if (isUsingMenu)
    {
        updateMenuScreen();
    } 
    else
    {
        updateTimerScreen();
    }
}

void resetGame()
{
    // Reset game to starting state
    p0.secondsRun += (p1.secondsRun + p2.secondsRun);
    p1.secondsRun = 0;
    p2.secondsRun = 0;
    activePlayer = &p0;
    isGameOver = false;
}

void updateTimerScreen()
{
    // Check for button presses
    if (analogRead(Write) < 512) { activePlayer = &p1; }  // Set player 1 active
    if (analogRead(Black) < 512) { activePlayer = &p2; }  // Set player 2 active

    switch (lcdKey)
    {
        case btnUP: { activePlayer = &p0; break; }    // Pause the timers
        case btnSELECT:
            {
                if (isGameOver)
                {
                    resetGame();
                }
                else
                {
                    // Activate the menu
                    isUsingMenu = true;
                    activePlayer = &p0;
                }
                delay(500);  // Delay required otherwise the button fires repeatedly
                break;
            }
        case btnNONE:  // Update timer if no button pressed
            {
                if (isGameOver) {
                    // Do nothing and wait for btnSELECT to reset the game
                }
                else
                {
                    // Check if either player is out of time
                    int loser = getLoserNumber();
                    
                    lcd.setCursor(0, 0);
                    if (loser > 0) {
                        lcd.print("   Game Over!   ");
                        lcd.setCursor(0, 1);
                        lcd.print(" Player " + String(loser) + " loses ");
                        isGameOver = true;
                    }
                    else
                    {
                        // Update timer display
                        lcd.print("P1 ChessClock P2");
                        lcd.setCursor(0, 1);
                        lcd.print(timeString(p1.SecondsRemaining()));
                        lcd.setCursor(5, 1);
                        lcd.print(activePlayer->playIndicator);
                        lcd.setCursor(11, 1);
                        lcd.print(timeString(p2.SecondsRemaining()));
                    }
                }
                break;
            }
    }
}

void updateMenuScreen()
{
    switch (lcdKey)
    {
        case btnUP:
            { menuPlayer->IncrementMinutes(); break; } // Increase minutes
        case btnDOWN:
            { menuPlayer->DecrementMinutes(); break; } // Decrease minutes
        case btnLEFT:
            {
                // Set pointer to player 1 as player to change time for
                menuPlayer = &p1;
                break;
            }
        case btnRIGHT:
            {
                // Set pointer to player 2 as player to change time for
                menuPlayer = &p2;
                break;
            }
        case btnSELECT:
            {
                // Return to timer screen
                isUsingMenu = false;
                delay(500);  // Delay required otherwise the button fires repeatedly
                break;
            }
        case btnNONE:
            {
                // Update menu display
                lcd.setCursor(0, 0);
                lcd.print("      MENU      ");
                lcd.setCursor(0, 1);
                String playerMins;
                playerMins = (menuPlayer->minutes < 10) ? " " + String(menuPlayer->minutes) : String(menuPlayer->minutes);
                lcd.print(menuPlayer->menuText + playerMins);
                break;
            }
    }
}

int getLoserNumber()
{
    // Return the number of the losing player, or 0 if both have time remaining
    if (p1.SecondsRemaining() <= 0) { return 1; }
    else if (p2.SecondsRemaining() <= 0) { return 2; }
    return 0;
}

String timeString(long seconds)
{
    // Convert seconds to an MM:SS display.
    int runMins = seconds / 60;
    int runSecs = seconds % 60;
    return pad(runMins) + ":" + pad(runSecs);
}

String pad(int toPad)
{
    // Zero-pad to ensure two chars in the string for mins/secs
    return (toPad < 10) ? "0" + String(toPad) : String(toPad);
}
