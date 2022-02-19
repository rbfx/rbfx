//
// Copyright (c) 2008-2022 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/Core/Timer.h>
#include <Urho3D/SystemUI/Console.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Engine/EngineEvents.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/UI/Button.h>

#include "ConsoleInput.h"

#include <Urho3D/DebugNew.h>

// Expands to this example's entry-point

// Hunger level descriptions
const char* hungerLevels[] = {
    "bursting",
    "well-fed",
    "fed",
    "hungry",
    "very hungry",
    "starving"
};

// Urho threat level descriptions
const char* urhoThreatLevels[] = {
    "Suddenly Urho appears from a dark corner of the fish tank",
    "Urho seems to have his eyes set on you",
    "Urho is homing in on you mercilessly"
};

ConsoleInput::ConsoleInput(Context* context) :
    Sample(context)
{
}

void ConsoleInput::Start()
{
    // Execute base class startup
    Sample::Start();

    // Subscribe to console commands and the frame update
    SubscribeToEvent(E_CONSOLECOMMAND, URHO3D_HANDLER(ConsoleInput, HandleConsoleCommand));
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(ConsoleInput, HandleUpdate));

    // Subscribe key down event
    SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(ConsoleInput, HandleEscKeyDown));
    UnsubscribeFromEvent(E_KEYUP);

    // Enable filesystem interaction in console.
    context_->GetSubsystem<FileSystem>()->SetExecuteConsoleCommands(true);

    // Show the console by default, make it large. Console will show the text edit field when there is at least one
    // subscriber for the console command event
    auto* console = GetSubsystem<Console>();
    console->SetCommandInterpreter(GetTypeName());
    console->SetVisible(true);

    // Show OS mouse cursor
    GetSubsystem<Input>()->SetMouseVisible(true);

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_FREE);

    // Open the operating system console window (for stdin / stdout) if not open yet
    OpenConsoleWindow();

    // Initialize game and print the welcome message
    StartGame();

    // Randomize from system clock
    SetRandomSeed(Time::GetSystemTime());
}

void ConsoleInput::HandleConsoleCommand(StringHash eventType, VariantMap& eventData)
{
    using namespace ConsoleCommand;
    if (eventData[P_ID].GetString() == GetTypeName())
        HandleInput(eventData[P_COMMAND].GetString());
}

void ConsoleInput::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    // Check if there is input from stdin
    ea::string input = GetConsoleInput();
    if (input.length())
        HandleInput(input);
}

void ConsoleInput::HandleEscKeyDown(StringHash eventType, VariantMap& eventData)
{
    // Unlike the other samples, exiting the engine when ESC is pressed instead of just closing the console
    if (eventData[KeyDown::P_KEY].GetInt() == KEY_ESCAPE)
    {
        GetSubsystem<Console>()->SetVisible(false);
        CloseSample();
    }
}

void ConsoleInput::StartGame()
{
    Print("Welcome to the Urho adventure game! You are the newest fish in the tank; your\n"
          "objective is to survive as long as possible. Beware of hunger and the merciless\n"
          "predator cichlid Urho, who appears from time to time. Evading Urho is easier\n"
          "with an empty stomach. Type 'help' for available commands.");

    gameOn_ = true;
    foodAvailable_ = false;
    eatenLastTurn_ = false;
    numTurns_ = 0;
    hunger_ = 2;
    urhoThreat_ = 0;
}

void ConsoleInput::EndGame(const ea::string& message)
{
    Print(message);
    Print("Game over! You survived " + ea::to_string(numTurns_) + " turns.\n"
          "Do you want to play again (Y/N)?");

    gameOn_ = false;
}

void ConsoleInput::Advance()
{
    if (urhoThreat_ > 0)
    {
        ++urhoThreat_;
        if (urhoThreat_ > 3)
        {
            EndGame("Urho has eaten you!");
            return;
        }
    }
    else if (urhoThreat_ < 0)
        ++urhoThreat_;
    if (urhoThreat_ == 0 && Random() < 0.2f)
        ++urhoThreat_;

    if (urhoThreat_ > 0)
        Print(ea::string(urhoThreatLevels[urhoThreat_ - 1]) + ".");

    if ((numTurns_ & 3u) == 0 && !eatenLastTurn_)
    {
        ++hunger_;
        if (hunger_ > 5)
        {
            EndGame("You have died from starvation!");
            return;
        }
        else
            Print("You are " + ea::string(hungerLevels[hunger_]) + ".");
    }

    eatenLastTurn_ = false;

    if (foodAvailable_)
    {
        Print("The floating pieces of fish food are quickly eaten by other fish in the tank.");
        foodAvailable_ = false;
    }
    else if (Random() < 0.15f)
    {
        Print("The overhead dispenser drops pieces of delicious fish food to the water!");
        foodAvailable_ = true;
    }

    ++numTurns_;
}

void ConsoleInput::HandleInput(const ea::string& input)
{
    ea::string inputLower = input.to_lower();
    inputLower.trim();

    if (inputLower.empty())
    {
        Print("Empty input given!");
        return;
    }

    if (inputLower == "quit" || inputLower == "exit")
        CloseSample();
    else if (gameOn_)
    {
        // Game is on
        if (inputLower == "help")
            Print("The following commands are available: 'eat', 'hide', 'wait', 'score', 'quit'.");
        else if (inputLower == "score")
            Print("You have survived " + ea::to_string(numTurns_) + " turns.");
        else if (inputLower == "eat")
        {
            if (foodAvailable_)
            {
                Print("You eat several pieces of fish food.");
                foodAvailable_ = false;
                eatenLastTurn_ = true;
                hunger_ -= (hunger_ > 3) ? 2 : 1;
                if (hunger_ < 0)
                {
                    EndGame("You have killed yourself by over-eating!");
                    return;
                }
                else
                    Print("You are now " + ea::string(hungerLevels[hunger_]) + ".");
            }
            else
                Print("There is no food available.");

            Advance();
        }
        else if (inputLower == "wait")
        {
            Print("Time passes...");
            Advance();
        }
        else if (inputLower == "hide")
        {
            if (urhoThreat_ > 0)
            {
                bool evadeSuccess = hunger_ > 2 || Random() < 0.5f;
                if (evadeSuccess)
                {
                    Print("You hide behind the thick bottom vegetation, until Urho grows bored.");
                    urhoThreat_ = -2;
                }
                else
                    Print("Your movements are too slow; you are unable to hide from Urho.");
            }
            else
                Print("There is nothing to hide from.");

            Advance();
        }
        else
            Print("Cannot understand the input '" + input + "'.");
    }
    else
    {
        // Game is over, wait for (y)es or (n)o reply
        if (inputLower[0] == 'y')
            StartGame();
        else if (inputLower[0] == 'n')
            CloseSample();
        else
            Print("Please answer 'y' or 'n'.");
    }
}

void ConsoleInput::Print(const ea::string& output)
{
    // Logging appears both in the engine console and stdout
    URHO3D_LOGINFO(output);
}
