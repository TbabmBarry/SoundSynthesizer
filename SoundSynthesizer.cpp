#include <iostream>
using namespace std;

#include "olcNoiseMaker.h"

double w(double dHertz)
{
    return dHertz * 2.0 * PI;
}

struct sEnvelopADSR
{
    double dAttackTime;
    double dDecayTime;
    double dReleaseTime;

    double dSustainAmplitude;
    double dStartAmplitude;

    double dTriggerOnTime;
    double dTriggerOffTime;

    bool bNoteOn;

    sEnvelopADSR()
    {
        dAttackTime = 0.100;
        dDecayTime = 0.01;
        dStartAmplitude = 1.0;
        dSustainAmplitude = 0.8;
        dReleaseTime = 0.200;
        dTriggerOnTime = 0.0;
        dTriggerOffTime = 0.0;
        bNoteOn = false;
    }

    double GetAmplitude(double dTime)
    {
        double dAmplitude = 0.0;

        double dLifeTime = dTime - dTriggerOnTime;

        if (bNoteOn)
        {
            // ADS

            // Attack
            if (dLifeTime <= dAttackTime)
                dAmplitude = (dLifeTime / dAttackTime) * dStartAmplitude;

            // Decay
            if (dLifeTime > dAttackTime && dLifeTime <= (dAttackTime + dDecayTime))
                dAmplitude = (dLifeTime - dAttackTime / dDecayTime) * (dStartAmplitude * dStartAmplitude) + dStartAmplitude;
            
            // Sustain
            if (dLifeTime > (dAttackTime + dDecayTime))
            {
                dAmplitude = dSustainAmplitude;
            }
        }
        else {
            // Release
            dAmplitude = ((dTime - dTriggerOffTime) / dReleaseTime) * (0.0 - dSustainAmplitude) + dStartAmplitude;
        }

        if (dAmplitude <= 0.0001)
        {
            dAmplitude = 0;
        }
        return dAmplitude;
    }

    void NoteOn(double dTimeOn)
    {
        dTriggerOnTime = dTimeOn;
        bNoteOn = true;
    }

    void NoteOff(double dTimeOff)
    {
        dTriggerOffTime = dTimeOff;
        bNoteOn = false;
    }
};

atomic<double> dFrequencyOutput = 0.0;
double dOctaveBaseFrequency = 110.0;
double d12thRootOf2 = pow(2.0, 1.0 / 12.0);
sEnvelopADSR envelope;


double osc(double dHertz, double dTime, int nType)
{
    switch (nType)
    {
    case 0: // sin wave
        return sin(w(dHertz) * dTime);
    case 1: // square wave
        return sin(w(dHertz) * dTime) > 0.0 ? 1.0 : -1.0;
    case 2: // triangular wave
        return asin(sin(w(dHertz) * dTime));
    case 3:
    {
        double dOutput = 0.0;
        for (double n = 1.0; n < 100.0; n++)
            dOutput += (sin(n * w(dHertz) * dTime)) / n;
        return dOutput * (2.0 / PI);
    }
    case 4:
        return (2.0 / PI) * (dHertz * PI * fmod(dTime, 1.0 / dHertz) - (PI / 2.0));
    case 5:
        return 2.0 * ((double)rand() / (double)RAND_MAX) - 1.0;
    default:
        return 0.0;
    }
}

double MakeNoise(double dTime)
{
    double dOutput = envelope.GetAmplitude(dTime) * osc(dFrequencyOutput, dTime, 3);
    
    return dOutput * 0.4; // Master Volume
}




int main()
{
    // Get all sound hardware
    vector<wstring> devices = olcNoiseMaker<short>::Enumerate();
    // Display finding
    for (auto &d : devices)
        wcout << "Found Output Device:" << d << endl;
    wcout << "Using Device: " << devices[0] << endl;

    // Display a keyboard
    wcout << endl <<
        "|   |   |   |   |   | |   |   |   |   | |   | |   |   |   |" << endl <<
        "|   | S |   |   | F | | G |   |   | J | | K | | L |   |   |" << endl <<
        "|   |___|   |   |___| |___|   |   |___| |___| |___|   |   |__" << endl <<
        "|     |     |     |     |     |     |     |     |     |     |" << endl <<
        "|  Z  |  X  |  C  |  V  |  B  |  N  |  M  |  ,  |  .  |  /  |" << endl <<
        "|_____|_____|_____|_____|_____|_____|_____|_____|_____|_____|" << endl << endl;

    olcNoiseMaker<short> sound(devices[0], 44100, 1, 8, 512);

    // Link noise function with sound machine
    sound.SetUserFunction(MakeNoise);

    while (1) 
    {
        // Add a keyboard like a piano
        int nCurrentKey = -1;
        bool bKeyPressed = false;
        for (int k = 0; k < 15; k++)
        {
            if (GetAsyncKeyState((unsigned char)("ZSXCFVGBNJMK\xbcL\xbe"[k])) & 0x8000)
            {
                if (nCurrentKey != k)
                {
                    dFrequencyOutput = dOctaveBaseFrequency * pow(d12thRootOf2, k);
                    envelope.NoteOn(sound.GetTime());
                    wcout << "\rNote On : " << sound.GetTime() << "s " << dFrequencyOutput << "Hz";
                    nCurrentKey = k;
                }
                
                bKeyPressed = true;
            }
        }

        if (!bKeyPressed)
        {
            if (nCurrentKey != -1)
            {
                wcout << "\rNote Off : " << sound.GetTime() << "s                        ";
                envelope.NoteOff(sound.GetTime());
                nCurrentKey = -1;
            }
        }
    }

    return 0;
}

