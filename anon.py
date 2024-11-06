import numpy as np
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Dense, Dropout
import nmap
import random
import pyttsx3
import speech_recognition as sr

class Anon:
    def __init__(self):
        self.name = "Anon"
        self.personality = self.define_personality()
        self.network = self.build_neural_network()
        self.nm = nmap.PortScanner()
        self.engine = pyttsx3.init()

    def define_personality(self):
        personalities = ["friendly", "aggressive", "neutral", "curious"]
        return random.choice(personalities)

    def build_neural_network(self):
        # Complex neural network model for advanced decision making
        model = Sequential()
        model.add(Dense(64, input_dim=10, activation='relu'))
        model.add(Dropout(0.3))
        model.add(Dense(128, activation='relu'))
        model.add(Dropout(0.3))
        model.add(Dense(64, activation='relu'))
        model.add(Dense(1, activation='sigmoid'))
        model.compile(loss='binary_crossentropy', optimizer='adam', metrics=['accuracy'])
        return model

    def learn(self, data, labels):
        self.network.fit(data, labels, epochs=15, batch_size=10)
        print(f"{self.name} has finished learning and feels {self.personality}!")
        self.speak(f"I have finished learning and feel {self.personality}.")

    def make_decision(self, input_data):
        prediction = self.network.predict(np.array([input_data]))[0][0]
        if self.personality == "aggressive":
            decision = "Attack" if prediction > 0.4 else "Monitor"
        elif self.personality == "curious":
            decision = "Analyze" if prediction > 0.3 else "Observe"
        else:
            decision = "Monitor" if prediction > 0.5 else "Ignore"
        print(f"{self.name} ({self.personality} mode) decided: {decision}")
        self.speak(f"I have decided to {decision}")
        return decision

    def scan_network(self, target='192.168.1.0/24'):
        print(f"{self.name} is scanning the network in {self.personality} mode...")
        scan_result = self.nm.scan(hosts=target, arguments='-sn')
        for host in self.nm.all_hosts():
            if 'mac' in self.nm[host]['addresses']:
                print(f"Host: {host} ({self.nm[host].hostname()}) - MAC: {self.nm[host]['addresses']['mac']}")
            else:
                print(f"Host: {host} ({self.nm[host].hostname()})")
        return scan_result

    def respond(self, message):
        responses = {
            "friendly": ["I'm here to help!", "At your service!", "Let me assist you."],
            "aggressive": ["Let's get things done!", "Time to take control!", "I'm ready to attack."],
            "neutral": ["What would you like me to do?", "I'm here.", "Awaiting instructions."],
            "curious": ["Tell me more!", "Interesting, let’s dig deeper.", "Let’s learn together!"]
        }
        response = random.choice(responses[self.personality])
        print(f"{self.name} ({self.personality}): {response}")
        self.speak(response)
        return response

    def greet(self):
        greeting = f"Hello, I am {self.name}, your {self.personality} cybersecurity assistant!"
        print(greeting)
        self.speak(greeting)

    def speak(self, text):
        # Text-to-speech function
        self.engine.say(text)
        self.engine.runAndWait()

    def listen(self):
        # Voice recognition function to interpret commands
        recognizer = sr.Recognizer()
        with sr.Microphone() as source:
            print("Listening...")
            audio = recognizer.listen(source)
        try:
            command = recognizer.recognize_google(audio).lower()
            print(f"You said: {command}")
            return command
        except sr.UnknownValueError:
            print("I couldn't understand that.")
            return None
        except sr.RequestError:
            print("Speech recognition service is unavailable.")
            return None

    def handle_command(self, command):
        # Parse and execute commands based on user input
        if command in ["scan network", "scan"]:
            self.scan_network()
        elif command in ["learn", "train"]:
            # Placeholder data for learning
            data = np.random.rand(100, 10)
            labels = np.random.randint(2, size=100)
            self.learn(data, labels)
        elif command in ["make decision", "decision"]:
            test_data = np.random.rand(10)  # Example input
            self.make_decision(test_data)
        elif command in ["how are you", "hello"]:
            self.respond("How are you?")
        else:
            self.speak("I'm not sure how to respond to that.")

# Example Usage
if __name__ == "__main__":
    anon = Anon()
    anon.greet()
    
    while True:
        print("\nPlease enter a command or say it aloud (e.g., 'scan network', 'make decision', 'how are you'): ")
        command = anon.listen() or input("> ").strip().lower()
        anon.handle_command(command)
        if command in ["exit", "quit", "stop"]:
            anon.speak("Goodbye!")
            break
