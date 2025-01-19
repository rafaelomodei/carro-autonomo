import RPi.GPIO as GPIO
import time

# Configuração dos pinos
LED_PIN = 27  # GPIO 17 (BCM)
LED_PIN2 = 17  # GPIO 17 (BCM)

# Configuração do GPIO
GPIO.setmode(GPIO.BCM)  # Usar numeração BCM
GPIO.setup(LED_PIN, GPIO.OUT)  # Configurar como saída
GPIO.setup(LED_PIN2, GPIO.OUT)  # Configurar como saída

try:
    print("Pisca LED - Pressione Ctrl+C para sair")
    while True:
        GPIO.output(LED_PIN, GPIO.HIGH)  # Liga o LED
        time.sleep(1.5)  # Espera 500 ms
        GPIO.output(LED_PIN2, GPIO.LOW)   # Desliga o LED
        time.sleep(1.5)  # Espera 500 ms
        GPIO.output(LED_PIN, GPIO.LOW)  # Liga o LED
        time.sleep(1.5)  # Espera 500 ms
        GPIO.output(LED_PIN2, GPIO.HIGH)   # Desliga o LED
        time.sleep(1.5)  # Espera 500 ms
except KeyboardInterrupt:
    print("\nEncerrando...")
finally:
    GPIO.cleanup()  # Limpa a configuração dos pinos
