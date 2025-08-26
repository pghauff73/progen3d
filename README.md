# ProGen3d
3d Procedural Generator

ProGen3d is a grammar that uses variables and symbolic math objects for calculations.
Based on https://github.com/santipaprika/procedural-buildings 
Uses glm vector math, texture loading, opengl.

See https://snapcraft.io/progen3d for ProGen3d 0.4a linux version.
See Release for 0.4a windows 11 version of ProGen3d.exe 

<a href="https://snapcraft.io/progen3d">
  <img alt="Get it from the Snap Store" src="https://snapcraft.io/static/images/badges/en/snap-store-black.svg" />
</a>

### Screencast

[Screencast from 25-11-23 15:44:57.webm](https://github.com/pghauff55/ProGen3d/assets/5868240/5273326b-75a2-41dd-84e0-dd5553f31625)


<div align="center">
    <img src="VisualGrammar-Editor-example.png" width="1200px"</img> 
</div>

### Screenshot
<div align="center">
    <img src="Screenshot36.png" width="1200px"</img> 
</div>


### Buildings 
<div align="center">
    <img src="Screenshot1515.png" width="1200px"</img> 
</div>

### Variables

{R | R*} variable_name ( min max )
---  Sets a variable

Example
```
#float
X -> R Height ( 4.6 10.7 )

#integer
Y -> R* xlen ( 4 6 )
```

### Rules

Rule repeat_variable variable_randomise1 variable_randomise2 ... ; probability -> {Start} | {Repeat_Body} | {End} -> Alternate_Rule

Example
```
X -> R Height ( 4 7 ) R Width (10 13) Y

Y 8 Width Height ; 0.8 -> | S ( Width Height Width ) I ( Cube ) | T ( Width+3 0 0 ) -> Empty

Empty ->
```

### Scale

S ( variable variable variable )
S ( numeric numeric numeric )
S ( calculation calculation calculation )

or any combination of
```
S ( Width-1.5 Height/2 Width*2-4.3 ) 
```

### Translate
T ( variable variable variable )
T ( numeric numeric numeric )
T ( calculation calculation calculation )

or any combination of

```
T ( -1.5 Height/2 1.0/Width ) 
```


### Instantiate
I ( Cube | CubeX | CubeY texindex rotate_multiple scaletex)

# Texture index
# Rotate Texture
# Scale Texture
Cube positions in the middle CubeX positions at the X edge and similarly with CubeY on the Y edge.
I ( Sphere )
I ( Cylinder )

## Repeat Sections

Rule Repeat Section by Repeat_variable converted to integer floor and reroll random values for variables var_1,var_2,... to var_n the repeat section, starting with the start section and ending with the end section.
```
Rule Repeat_variable var_1 var_2 ... var_n  -> Start Section  | Repeat Section  | End Section 
```
Rule Repeat second section by variable Xlength converted to integer floor starting with the start section and ending with empty section.
```
WindowsXN Xlength ->  T ( Xlength/2.0-0.8 0 -1 )   | T ( -0.9 0 0 ) [  S ( 0.8 0.95 0.01 ) T ( 0.9 0 0 ) I ( Cube ) ] 
```

### Probability

The probability that Rule will be the first section set of sections or the second set of sections

```
Rule var_1 var_2 ... var_n ; p -> Sections -> Sections
```

The probability is 60% of drawing a building or empty.
```
AddBuilding ; 0.6 -> T ( 0 0 3 ) [ Center ]  T ( 0 0 -2 )  [ SectionX  ] [ SectionY ]
```

An example of a more complex probabilty construct.

```
Ruke repeat  {initialise variables} -> | ChooseX  ChooseY


ChooseX ; 0.2 -> [ Rules I ( type texindex ) ] -> ChooseY

ChooseY ; 0.8 -> [ Rules I ( type texindex ) ]-> [ Rules I ( type texindex ) ]

```

### Example
```
X -> R CenterHeight ( 4 10 ) R Xlength ( 6 8 ) R TowerHeight ( 6 14 ) R BaseHeight ( 2 3 ) Y


Y -> [ Floor ] T ( -50 0 -50 ) [  City ]
City ->  Street Street Street Street Street Street
Street ->  T ( 0 0 15 ) StreetBlocks 
Floor -> S ( 120 0.1 120 ) I ( Cube )
StreetBlocks ->  [ AddBuilding ] [ MoreBlocks ] 
MoreBlocks 8 ->    T ( 10 0 0 )  [ AddBuilding ]  



AddBuilding ; 0.6 -> T ( 0 0 3 ) [ Center ]  T ( 0 0 -2 )  [ SectionX  ]

Center 1 TowerHeight->   S ( 2 TowerHeight 2 ) I ( Cube )  Top 
Top ->  T ( 0 1 0 ) [ S ( 1 0.1 1 ) I ( Sphere ) ] T ( 0 -1 0 )  





Base 1 BaseHeight->   S ( Xlength  BaseHeight  4 )  I ( Cube )

BodyX ->  T ( 0 BaseHeight 0 ) S ( Xlength  CenterHeight  2 )  I ( Cube )  

Xend ->   S (  2  CenterHeight 2 ) T ( 1.75 0 0 ) I ( Cylinder ) 


SectionX CenterHeight CenterHeight Xlength ->   [ Base ] [ BodyX ] [ Xend ] | [ WindowsX ] T ( 0 1 0 ) | 
WindowsX Xlength ->  T ( Xlength/2.0-0.8 0 -1 )   | T ( -0.9 0 0 ) [  S ( 0.8 0.95 0.01 ) T ( 0.9 0 0 ) I ( Cube ) ] | 
```

<div align="center">
    <img src="snapshot01.png" width="1200px"</img> 
</div>






## Example 2 Textures




```

X -> R CenterHeight ( 4 10 ) R Xlength ( 6 8 ) R Ylength ( 4 6 ) R TowerHeight ( 6 14 ) R BaseHeight ( 2 3 ) Y


Y -> [ Floor ] T ( -50 0 -50 ) [  City ]
City ->  Street Street Street Street Street Street
Street ->  T ( 0 0 15 ) StreetBlocks 
Floor -> S ( 120 0.1 120 ) I ( Cube )
StreetBlocks ->  [ AddBuilding ] [ MoreBlocks ] 
MoreBlocks 8 ->    T ( 10 0 0 )  [ AddBuilding ]  



AddBuilding ; 0.6 -> T ( 0 0 3 ) [ Center ]  T ( 0 0 -2 )  [ SectionX  ] [ SectionY ]

Center 1 TowerHeight ->   S ( 2 TowerHeight 2 ) I ( Cube )  Top 
Top ->  T ( 0 1 0 ) [ S ( 1 0.1 1 ) I ( Sphere ) ] T ( 0 -1 0 )  

Base 1 BaseHeight ->   S ( Xlength  BaseHeight  Ylength )  I ( Cube )

BodyX ->  T ( 0 BaseHeight 0 ) S ( Xlength  CenterHeight  2 )  I ( Cube )  
BodyY ->  T ( 0 BaseHeight 0 ) S ( 2  CenterHeight  Ylength )  I ( Cube )  

Xend ->   S (  2  CenterHeight 2 ) T ( 1.75 0 0 ) I ( Cylinder ) 
Yend ->   S (  2  CenterHeight 2 ) T ( 0 0 1.75 ) I ( Cylinder ) 

SectionX CenterHeight CenterHeight Xlength ->   [ Base ] [ BodyX ] [ Xend ] | [ WindowsXN ] [ WindowsXS ] T ( 0 1 0 )  
WindowsXN Xlength ->  T ( Xlength/2.0-0.8 0 -1 )   | T ( -0.9 0 0 ) [  S ( 0.8 0.95 0.01 ) T ( 0.9 0 0 ) I ( Cube ) ]   
WindowsXS Xlength ->  T ( Xlength/2.0-0.8 0 1 )   | T ( -0.9 0 0 ) [  S ( 0.8 0.95 0.01 ) T ( 0.9 0 0 ) I ( Cube ) ]   

SectionY CenterHeight CenterHeight Ylength ->   [ BodyY ] [ Yend ] | [ WindowsYE ] [ WindowsYW ]  T ( 0 1 0 )  
WindowsYE Ylength ->  T ( -1 0 Ylength/2.0-0.8 )   | T ( 0 0 -0.9 ) [  S ( 0.01 0.95 0.8 ) T ( 0 0 0.9 ) I ( Cube ) ]  
WindowsYW Ylength ->  T ( 1 0 Ylength/2.0-0.8 )   | T ( 0 0 -0.9 ) [  S ( 0.01 0.95 0.8 ) T ( 0 0 0.9 ) I ( Cube ) ]
```


<div align="center">
    <img src="Screenshot-1.png" width="1200px"</img> 
</div>


### Simple Procedural Building
```
X -> R xlen ( 4 6 ) R ylen ( 2 3 )  R WindowGapSides ( 0.2 0.3 ) R WindowGapTop ( 0.1 0.2 ) R WindowGapBottom ( 0.35 0.5 ) S ( 2.4 2.4 2.4 ) Y



Y  6 xlen ylen -> [ S ( 2*xlen 0.1 2*ylen ) I ( Cube 16 ) ] [ T ( -xlen 0 ylen ) GenX ]  [ T ( -xlen 0 -ylen ) GenX ] T ( 0 1 0 )




GenX xlen  WindowGapSides WindowGapTop WindowGapBottom -> | ChooseBlankWallX T ( 1 0 0 ) ChooseDoorWindowX T ( 1 0 0 )


ChooseBlankWallX ; 0.2 -> [ S ( 1 1 0.1 ) I ( CubeX 17 ) ] -> WindowDoorWindowX

ChooseDoorWindowX ; 0.8 -> WindowDoorSectionX -> WindowSectionX





WindowSectionX -> [ S ( 1 WindowGapBottom 0.1 ) I ( CubeX 17 ) ] [ T ( 0 1-WindowGapTop 0 ) S ( 1 WindowGapTop 0.1 ) I ( CubeX 17 ) ] [ T ( 0 WindowGapBottom 0 ) S ( WindowGapSides 1-WindowGapTop-WindowGapBottom 0.1 ) I ( CubeX 17 ) ] [ T ( 1-WindowGapSides WindowGapBottom 0 ) S ( WindowGapSides 1-WindowGapTop-WindowGapBottom 0.1 ) I ( CubeX 17 ) ]
WindowDoorSectionX -> [ S ( 1 WindowGapTop/2 0.1 ) I ( CubeX 17 ) ] [ T ( 0 1-WindowGapTop 0 ) S ( 1 WindowGapTop 0.1 ) I ( CubeX 17 ) ] [ T ( 0 WindowGapTop/2 0 ) S ( WindowGapSides 1-WindowGapTop-WindowGapTop/2 0.1 ) I ( CubeX 17 ) ] [ T ( 1-WindowGapSides WindowGapTop/2 0 ) S ( WindowGapSides 1-WindowGapTop-WindowGapTop/2 0.1 ) I ( CubeX 17 ) ]


```



<div align="center">
    <img src="Screenshot32.png" width="1200px"</img> 
</div>


```
X -> R xlen ( 4 6 ) R ylen ( 2 3 )  R WindowGapSides ( 0.2 0.3 ) R WindowGapTop ( 0.1 0.2 ) R WindowGapBottom ( 0.35 0.5 ) S ( 2.4 2.4 2.4 ) Y



Y  6 xlen ylen ->  [ Floor ] [ T ( -xlen 0 ylen ) GenX ]  [ T ( -xlen 0 -ylen ) S ( 1 1 -1 ) GenX ] T ( 0 1 0 )



Floor -> [ S ( 2*xlen 0.1 2*ylen ) I ( Cube 16 ) ] [ FenceY ]



GenX xlen  WindowGapSides WindowGapTop WindowGapBottom -> | ChooseBlankWallX T ( 1 0 0 ) ChooseDoorWindowX T ( 1 0 0 )


ChooseBlankWallX ; 0.2 -> [ S ( 1 1 0.1 ) I ( CubeX 17 ) ] -> WindowDoorWindowX

ChooseDoorWindowX ; 0.8 -> WindowDoorSectionX -> WindowSectionX





WindowSectionX ->  WindowTop  WindowBottom  WindowLeftSide WindowRightSide


WindowTop -> [ S ( 1 WindowGapBottom 0.1 ) I ( CubeX 17 ) ] [ T ( WindowGapSides WindowGapBottom 0.1 ) S ( 1-2*WindowGapSides 0.05 0.025 ) I ( CubeX 3 ) ]
WindowBottom -> [ T ( 0 1-WindowGapTop 0 ) S ( 1 WindowGapTop 0.1 ) I ( CubeX 17 ) ] [ T ( WindowGapSides 1-WindowGapTop 0.1 ) S ( 1-2*WindowGapSides 0.05 0.025 ) I ( CubeX 3 ) ]
WindowLeftSide -> [ T ( 0 WindowGapBottom 0 ) S ( WindowGapSides 1-WindowGapTop-WindowGapBottom 0.1 ) I ( CubeX 17 ) ] [ T ( WindowGapSides WindowGapBottom 0.1 ) S ( 0.05 1-WindowGapTop-WindowGapBottom 0.025 ) I ( CubeX 3 ) ]
WindowRightSide -> [ T ( 1-WindowGapSides WindowGapBottom 0 ) S ( WindowGapSides 1-WindowGapTop-WindowGapBottom 0.1 ) I ( CubeX 17 ) ] [ T ( 1-WindowGapSides-0.05 WindowGapBottom 0.1 ) S ( 0.05 1-WindowGapTop-WindowGapBottom 0.025 ) I ( CubeX 3 ) ]


WindowDoorSectionX -> Top SideRight SideLeft Bottom Verandah


Verandah -> [ T ( 0.5 0 0 ) S ( 1 0.025 1 ) I ( Cube 16 ) ] [ Fence ]
Top -> [ S ( 1 WindowGapTop/2 0.1 ) I ( CubeX 17 ) ] [ T ( WindowGapSides 0.025 0.1 ) S ( 1-2*WindowGapSides 0.05 0.0125 ) I ( CubeX 12 ) ] 
Bottom -> [ T ( 0 1-WindowGapTop 0 ) S ( 1 WindowGapTop 0.1 ) I ( CubeX 17 ) ] [ T ( WindowGapSides 1-WindowGapTop-0.025 0.1 ) S ( 1-2*WindowGapSides 0.05 0.0125 ) I ( CubeX 12 ) ] 
SideLeft -> [ T ( 0 WindowGapTop/2 0 ) S ( WindowGapSides 1-WindowGapTop-WindowGapTop/2 0.1 ) I ( CubeX 17 ) ] [ T ( WindowGapSides WindowGapTop/2 0.1 ) S ( 0.05  1-WindowGapTop-WindowGapTop/2  0.0125 ) I ( CubeX 12 ) ]
SideRight -> [ T ( 1-WindowGapSides WindowGapTop/2 0 ) S ( WindowGapSides 1-WindowGapTop-WindowGapTop/2 0.1 ) I ( CubeX 17 ) ] [ T ( 1-WindowGapSides-0.05 WindowGapTop/2 0.1 ) S ( 0.05  1-WindowGapTop-WindowGapTop/2  0.0125 ) I ( CubeX 12 ) ]




Fence 6 ->   [ T ( 0.5 0.1 0.5 ) S ( 1 0.0125 0.0125 ) I ( Cube 15 ) ] [ T ( 0.5 0.4 0.5 ) S ( 1 0.0125 0.0125 ) I ( Cube 15 ) ] | [ T ( 0 0 0.5 ) S ( 0.0125 0.4 0.0125 ) I ( Cube 15 ) ] T ( 1/6 0 0 ) | [ T ( 0 0 0.5 ) S ( 0.0125 0.4 0.0125 ) I ( Cube 15 ) ] 



FenceY ylen*2 ->   [ T ( 0.5 0.1 0.5 ) S ( 0.0125 0.0125 1 ) I ( Cube 15 ) ] [ T ( 0.5 0.4 0.5 ) S ( 0.0125 0.0125 1 ) I ( Cube 15 ) ] | [ T ( 0.5 0 0 ) S ( 0.0125 0.4 0.0125 ) I ( Cube 15 ) ] T ( 0 0 1/6 ) | [ T ( 0.5 0 0 ) S ( 0.0125 0.4 0.0125 ) I ( Cube 15 ) ] 

```


<div align="center">
    <img src="Screenshot33.png" width="1200px"</img> 
</div>
