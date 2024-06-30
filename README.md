# Курсовая работа
Разработать собственную систему сбора информации для «умного дома» (температура, влажность,
загазованность, потребление энергии и т. д – по выбору обучающегося) ,
построенную на основе протокола обмена MQTT

Датчик записывает в отдельный файл массив данных, состоящий из даты
измерения и величину температуры.
Издатель считывает текстовый файл и отправляет его в среду разработки Node-Red
по протоколу обмена MQTT через фиксированный временной интервал.
Подписчик подписан на среду Node-Red и считывает значения датчика. Подписчик
записывает полученные данные в файл с указанием времени и даты записи.
Программы издателя и подписчика работают одновременно.

Предполагается, что пользователь может с консоли самостоятельно задавать:  
● IP-адрес подключения,  
● логин и пароль подключения,  
● имена файлов, в которые будут записаны показания датчиков.  
## Скрины выполнения программы
