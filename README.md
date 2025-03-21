# TraceAndSweepCollision
Detecting collisions using synchronous and asynchronous line trace and sweep trace to avoid paper bullet problem [C++, UE4, UE5, Plugin]

If you want to know how I made this plugin and learn more about the technical details or watch devlog about it
[![Youtube Video Link](https://drive.google.com/uc?export=view&id=1ovjfXNPQOR1YU4GzjQ-e7VfbIfuPt_pc)](https://youtu.be/V5T8w7LT_WE)

# How to use
- Copy the folder TraceAndSweepCollision into plugins folder of your project and restart the editor
- Once editor opens go to Edit -> Plugins and under Project -> Gameplay you can see TraceAndSweepCollision. Enable the plugin and restart the editor
![image](https://drive.google.com/uc?export=view&id=1X6CJztNBE462l9sqr1y-gz_tJpxNSaif)
- Inside your actor that you want to replace collider with the plugin, delete the collider and add Trace And Sweep Collision component. (If using C++, add the component using CreateDefaultSuboject in constructor)
- **Drag and drop the Trace And Sweep Collision Manager actor into your level. Every level should have one of this actor in them. Otherwise the collisions will not work in that level**


## Adding points to track using Line Tracing
- If you want to use line trace then add child scene object component to Trace And Sweep Collision and set the scene object component location to desired location. You can add as many scene object components as you like. (Don't forget to disable the tick of these scene objects as its not needed and keeping it enabled is just waste of performance.)
Eg: For a bullet that needs single line trace
![image](https://drive.google.com/uc?export=view&id=1TxWdz8KMH9bmsNPnOoaLmaWDy1t1NbFv)
For a sword that needs more points
![image](https://drive.google.com/uc?export=view&id=1T9KL5P1GSwqmIzDu-TKVHKeLc5nHcZ4M)

**NOTE:** Right now if you want to see the debug drawing the location of these objects you have to add them to Trace And Sweep Collision in constructor. The functionality will not effect in game, but it will easier to see them in viewport if they are added to the component.
![image](https://drive.google.com/uc?export=view&id=16IrZzb4Xbd2eAEL-Ynf_nMYhbmRcIbJc)

- If its not a static mesh but a skeletal mesh and you want to use bones as the points of to track, then the component **has** to be child of the skeletal mesh and you can add all bone names in component details panel.
![image](https://drive.google.com/uc?export=view&id=1JD3kkgcPwmB_lIfvSqCAyr6_f3ZeaVN4)


## Adding Shapes for Sweep Tracing
- Change the Trace style type to sweep and it will give you an array to add any nymber of shapes you want.
![image](https://drive.google.com/uc?export=view&id=1Ynbfu3KlpMdC5qmvk566ARyvTnKciN_J)
- This allows you to add any of three basic shape types, box, capsule and sphere. 
- In this sword example I wanted to add a rectangle that covers the entire sword and then a sphere that covers the hilt of the sword.
![image](https://drive.google.com/uc?export=view&id=1HM9bGhLWNPpA7nWPlWtgVukGGoHho9Yu)

> [!NOTE]
> Use the offset value for each shape to move and rotate the individual shapes. Right now selecting any shape will select entire component. I'm working on being able to select individual shapes and be able to move and rotate them inside the viewport. For now you have to just type the location and rotation in the offset for the shape.

> [!IMPORTANT]
> Due to how trace works in unreal there are some constraints on how scale is applied to the shapes.
> - For sphere shape: The maximum scale value is selected and applied on all axis. Eg. if scale is (3.0, 4.0, 2.0) then effective scale is 4.0 on all axis for sphere.
> - For capsule shape: Maximum scale value on X and Y axis is applied to X and Y axis. Z axis scale is independent. **World scale is also constrainted by half height. Radius cannot exceed half height.**
>  - Eg: If capsule radius is 10 and half height is 20. And world scale is (3.0, 3.0, 1.0) radius should become 30, but since half height is 20, radius will be restricted to 20. 

## Setting up collisions
- Every way unreal allows trace is available as drop down options in the component. These are same options that are available when you want to do a trace in blueprint or C++.
- Execution Type: Whether you want the trace to be asynchronous or synchronous.
![image](https://drive.google.com/uc?export=view&id=1FH11heSbbH-ThOgPpCc3ZEYgtbln5Di2)
- Trace Type: Whether you want the trace to be single or multi. 
![image](https://drive.google.com/uc?export=view&id=1OLDSV72M5EqJMaX7-7YRo_WKylxo-Bdl)
- Channel Type: Type of channel to use for tracing. There are multiple ways trace can happen. Trace Channel, Object Channel or Collision preset. (defined under Collisions of Project settings)
![image](https://drive.google.com/uc?export=view&id=1aoxkaOABpq8WIoj5uxUREe8psJRVnZdW)
- Trace Channel will give list of all trace channels you have in your project
![image](https://drive.google.com/uc?export=view&id=1IgzaYaEN5ePYRdp7Y-OMKwI3WHgHYfL9)
- Object Channel will let you add as many object channels as you want the trace to happen
![image](https://drive.google.com/uc?export=view&id=1odi_dFUvNBoN8Oin9J2NxCuZVB2Ez6EY)
- And Collision preset has list of all collision presets that are defined in your project.
![image](https://drive.google.com/uc?export=view&id=1iEWB1XKeZE0FAuHxFDLW38jJkcR_WN6F)

## Registering to begin and end overlap events.
- Line any primitive component, this component also comes with the same begin and end overlap. They behave same as begin and end overlap of colliders. So you can just replace replace the begin and end overlap of your collision component with these ones. Same with C++, just register to the begin and end overlap of TraceAndSweepCollision component the same way you register to any component.
![image](https://drive.google.com/uc?export=view&id=1cRR51qp_UtH-tZDY-4xmJT6dWqsdsUCH)

- Regarding the other actors and components that you have in your game (like walls, enemies etc), nothing needs to change on them. On being and end overlaps events will be generated the same way it was happening before. So the only change in your project that needs to happen is using this component instead of a collider.

- For foliage and instanced static meshes, this will still work and will give the correct other body index that you can use for your foliage and instanced static meshes.


## Debugging
- If you open the "Advanced" options there are many options that you can use to debug this component.
- You can set the colors for drawing the shapes and tracing the path and hit points etc. Set how long they stay on the screen and thickness etc.
![image](https://drive.google.com/uc?export=view&id=14VtjuAfUyegaIV-QepqH7_AVBnXMKliD)

Example look of how it will look when enabling these debug settings
![image](https://drive.google.com/uc?export=view&id=168iotgn35hNRnBUxVZ_283VwStVe1ceD)
![image](https://drive.google.com/uc?export=view&id=1sJsKwAQQV3-AYh07-jREuP8cPYGGwjXx)


