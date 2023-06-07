
## zephyr
```
1. keys binding: dts\bindings\input\gpio-keys.yaml
2. pwm driver: .dst->&pwm0 
```

## ble
```
1.连接参数更新: L2CAP如果由从机发起,然后走链路层更新连接参数流程
            如果由主机发起,直接走链路层流程即可.

2.启用加密: 由主机发起,三次握手后生效; 如果要重启加密,则主机发起三次握手停止加密,然后主机再发起加密流程.

```

## nrf
```
1.EasyDMA is a module implemented by some peripherals to gain direct access to Data RAM.
```
