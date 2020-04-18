# Modular

## About this project

This is attempt to create yet another IoT system based on chip components. The 
system assumed to be consists of small controllers based on simple
microcontrollers. Just name here such controllers modules. Each module has
simple limited functionality like handle button press, control the lamp or
movement sensor. 

Modules assumed to be grouped into blocks with one special transport module.
Communication withing block may be implemented with i2c and transport module 
will send data to other modules.

## Protocol

This is description of first draft. It may be changed in the future and this
readme may be not updated occasionally.

The project has stateless message-based binary protocol. There three types of
messages: Module description, event and method call. Each module has its own
2 byte long ID.

* The module description message tells everyone the modules name, its methods
  and events it can emits.

* The event message consists of emitter module's ID and arguments list.

* The method call message consists of module's ID which method should be called
  and arguments

Each argument above is one byte long.

### Event to method mapping

The common protocol part is in modular.h and modular.c files. There is special
event to method mapping functionality. Some server with global control logic may
say to lamp controller #2 to switch light on button #5 switch event. That is
mean "lamp #2 should call method switch\_light() when it received event 
button\_changed from button #5 module and pass event 'state' argument as 'state'
argument to method".

The modular library implements such functionality but user have to organise the
binary storage by himself.

### Code generator

There are prepared simple code generator for protocol's modules. You can
describe your module's methods and events in yaml file and generator will create
\*.c and \*.h files for you. Next there only transport functions and methods 
left to create.

## Project structure

Project uses cmake to build the project. To simplify build the cross-build.sh 
script prepared. It will produce the result in the build folder. Use -u for
usage.

The code generator is written on python and placed to pyhon/modular filder. 
The module\_example.yamp example placed there too. You can use gen\_module.sh
script to call it, or you can use generate\_modules function in cmake to produce
special targets for generated file. You can use tests as examples. The cmake-avr
git submodule used as toolchain to compile avr-specific code.
