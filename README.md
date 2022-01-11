# MyAirScanner

Wrządzenia do pomiaru jakości powietrza, które wyniki pomiarów przesyła za pośrednictwem ramek rozgłoszeniowych Bluetooth Low Energy (BLE). Główną jednostką urządzenia jest układ ESP32-SOLO-1 firmy Espressif Systems. Urządzenie wykonuje pomiary pyłów zawieszonych za pomocą sensora Plantower PMS5003 oraz temperatury i wilgotności za pomocą sensora DHT22. Dzięki zastosowaniu transmisji wyników za pośrednictwem ramek rozgłoszeniowych BLE, dane są odbierane w sposób bezpołączeniowy oraz mogą być odebrane przez nieograniczoną liczbę urządzeń jednocześnie. Aby była możliwa transmisja większej ilości danych niż pozwala na to pojedyncza ramka rozgłoszeniowa BLE został zastosowany cykliczny mechanizm zamiany ramek. Oprócz przesyłania wyników pomiarów urządzenie sygnalizuje jakość powietrza za pomocą ustawienia odpowiedniego koloru świecenia LED RGB. Urządzenie jest zasilane za pośrednictwem USB w standardzie 2.0, co umożliwia jego proste używanie w sposób stacjonarny oraz mobilny.
