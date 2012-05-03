#include "adc.h"
#include "serial.h"
#include "filter.h"

//-----------------globale Variablen-----------------

uint8_t Samples = 0; //Anzahl noch zu erstellender Samples

uint8_t triggerSet = 0; //gibt an ob bereits getriggert wurde

trigger_t triggerEvent = NONE; //gibt an ob auf fallende oder steigende Flanke getriggert wird

int lastValue = 0; //das letzte gemessene Sample

int triggerValue = 0; //auf diesen Wert soll getriggert werden

//-----------------Interrupt-Routinen----------------

//################################################
//#Wenn dieser Interrupt ausgeloest wird, loescht# 
//#er das Output-Compare-Match-Flag A. Der Timer #
//#kann nun erneut starten.                      #
//################################################
ISR(TIMER1_COMPA_vect) {
}

//################################################
//#Wenn dieser Interrupt ausgeloest wird, loescht# 
//#er das Output-Compare-Match-Flag B. Der ADU   #
//#kann nun erneut getriggert werden.            #
//################################################
ISR(TIMER1_COMPB_vect) { 
}

//################################################
//#Diese Routine liesst den ADU aus, zieht einen #
//#Offset ab und speichert den Wert.             #
//################################################
ISR(ADC_vect) {

	int result = ADC; //liesst ADU aus
	result = result - 512; //zieht Offset ab

	if(triggerSet==0){
		if(triggerEvent==RISING){
			if(lastValue<triggerValue){
				if(result>=triggerValue){triggerSet=1;}
			}
		} else {
			if(lastValue>triggerValue){
				if(result<=triggerValue){triggerSet=1;}
			}
		}
	}

	if(triggerSet==1){
		if(Samples>0){ //ueberprueft ob weiter Samples benoetigt werden
			filterWrite2Buf(result); //speichert Sample
			Samples--; //Anzahl benoetigter Samples decrementieren
		}
	}

	lastValue = result; //speichert letzten Messwert	
}

//---------------------Funktionen--------------------

//################################################
//#Diese Funktion initialisiert den ADU.         #
//#Verwendet werden die interne Referenz und der #
//#Autotriggermodus.                             #
//################################################
void adcInit() {

	ADMUX |= (1<<REFS1) | (1<<REFS0); //interne 2,56V Referenz
	ADCSRA |= (1<<ADIE) | (1<<ADPS2) | (1<<ADPS0); //ADU-Enable und FCPU:32
	ADCSRA |= (1<<ADATE); //Autotrigger mode

	ADCSRB |= (1<<ADTS2) | (1<<ADTS0); //Wandlung auf Comparematch Timer 1B
	ADCSRA |= (1<<ADEN); //ADU starten

	ADCSRA |= (1<<ADSC); //Dummywandlung
}

//################################################
//#Hier wird eine Messung gestartet. Die Anzahl  #
//#der Samples und die Samplerate werden hier    # 
//#festgelegt.                                   #
//################################################
void adcStart(uint16_t sampleRateCode, uint32_t sampleCount, trigger_t triggerMode, int16_t triggerLevel) {
	
	triggerEvent = triggerMode; //setzt Triggermodus
	triggerValue = triggerLevel; //setzt Triggerlevel

	if(triggerMode==NONE){ //im Fall von Triggermode = NONE wird der Triggerimpuls sofort gesetzt 
		triggerSet = 1;
	} else {
		triggerSet = 0;
	}

	TCCR1B |= (1<<WGM12); //CTC auf OutputcompareA
	OCR1A = sampleRateCode; //dient dem CTC zum resetten
	OCR1B = sampleRateCode; //loesst den Autotrigger aus

	TCNT1 = 0x0000; //Timer resetten
	TCCR1B |= (1<<CS20); //Timer1 mit CPU-Takt
	Samples = sampleCount;//Sampleanzahl resetten

}

//################################################
//#Diese Funktion gibt an ob gerade eine Messung #
//#durchgefuehrt wird.                           #
//################################################
uint8_t adcIsRunning() {

	if (Samples == 0){return 0;} //Messung laeuft noch
	return 1; //Messung beendet
}
