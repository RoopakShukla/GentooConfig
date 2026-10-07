# vis-tasks
A simlpe todo list manager

Create and manage tasks in plain text with tags:
### new task `<space>tn`![new](https://user-images.githubusercontent.com/251334/164336517-f3a906e0-985d-4b42-adc5-1a9ca4e997cd.gif)
### done `<space>td`![done](https://user-images.githubusercontent.com/251334/164336535-fcc97a74-51ee-4ce8-aae0-9ac35de2e521.gif)
### canceled `<space>tc`![canceled](https://user-images.githubusercontent.com/251334/164336741-537efd3a-f83e-47ab-9c44-6c8c32280acf.gif)
### blocked `<space>tb`![blocked](https://user-images.githubusercontent.com/251334/164336841-34700ac1-9ca6-4429-b939-037ef2eab76a.gif)

```
 - □ todo
 - ✔ done                                                       @done 2022-04-20
 - ✘ canceled                                               @canceled 2022-04-20
 - ⚠ blocked                                                 @blocked 2022-04-20
```

## setup

Load and enable the plugin in ![visrc.lua](doc/visrc.lua).
![setup](https://user-images.githubusercontent.com/251334/164337147-8f978def-9264-432e-a569-932ce7df3ccc.gif)

## Customize symbol and keymaps

Optionally pass a table `t` to `setup(t)` for custom keymaps and/or symbols.
This will create a map for each table entry.

Entry format: `<key> <symbol> <tag> <help>`

```
t = {
        {' tb', '⚠', 'blocked' , 'task: toggle blocked'},
        {' tc', '✘', 'canceled', 'task: toggle canceled'},
        {' td', '✔', 'done'    , 'task: toggle done'},
}
```
