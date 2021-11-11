from socket import *
import sys

# Адрес центральной платы
host = '192.168.20.130'
port = 81
addr = (host, port)

tcp_socket = socket(AF_INET, SOCK_STREAM)
tcp_socket.connect(addr)

#data = input('write to server: ')
#if not data:
#    tcp_socket.close()
#    sys.exit(1)

# encode - перекодирует введенные данные в байты, decode - обратно
#data = str.encode(data)

# Режим чтения или записи (2-запись 1-чтение)
mod = 2
# количество байт для записи или чтения
N = 1
# адрес устройства
Addr1 = 4
# адрес регистра
AddrReg = 1
# данные для записи
wdata = 1

# data = int(data)
# конвертация числа в тип bytes (2 байта)
data = mod.to_bytes(1, byteorder='big')
data += N.to_bytes(1, byteorder='big')
data += AddrReg.to_bytes(1, byteorder='big')
data += Addr1.to_bytes(1, byteorder='big')
data += wdata.to_bytes(2, byteorder='big')

tcp_socket.send(data)
data = bytes.decode(data)
data = tcp_socket.recv(1024)
print(data)

tcp_socket.close()
