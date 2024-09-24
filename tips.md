```
1.用app.overlay则默认使用,用其他文件名则要在build配置中添加
2.添加文件要重新保存cmakelists,此时FILE(GLOB app_sources src/*.c)会添加新文件
```

```
设备树
1.compatible字段用来binding驱动,yaml文件会要求此节点必备的属性
2.status字段为okay则表示此设备驱动启用,用disable则禁用掉
```
