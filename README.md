<p align="center">
    <br/>
    A new programming language built upon the <a href="https://craftinginterpreters.com">crafting interpreters tutorial</a>
    <br/>
    <br/>
    <a href="https://discord.gg/yg4RwQ6JRv">
        <img src="https://img.shields.io/discord/731577337686130858?logo=discord">
    </a>
</p>

# Sadie
Sadie is a fast scripting language built upon the crafting interpreters tutorial by Bob Nystrom

## Developers
[@classerase](https://github.com/classerase)

## Example
```c
enum RGB {
  RED = "red",
  GREEN = "green",
  BLUE = "blue"
}

fun getColour(colour) {
  if (colour == RGB.RED) {
    print("Red: 255, 0, 0");
  } else if (colour == RGB.GREEN) {
    print("Green: 0, 255, 0");
  } else if (colour == RGB.BLUE) {
    print("Blue: 0, 0, 255");
  } else {
    print("Unknown colour '" + colour + "'.");
  }
}

getColour("red"); // outputs Red: 255, 0, 0
getColour("yellow"); // outputs Unknown colour 'yellow'.
```
## Legal stuff
Sadie is licensed under the GNU GPLv3 license. see [LICENSE](https://github.com/sadie-lang/Sadie/blob/master/LICENSE)
