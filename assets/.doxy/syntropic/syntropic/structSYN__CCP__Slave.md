

# Struct SYN\_CCP\_Slave



[**ClassList**](annotated.md) **>** [**SYN\_CCP\_Slave**](structSYN__CCP__Slave.md)



_CCP Slave Instance Handle._ 

* `#include <syn_ccp.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  uint8\_t | [**active\_cal\_page**](#variable-active_cal_page)  <br> |
|  [**bool**](syn__defs_8h.md#enum-bool) | [**connected**](#variable-connected)  <br> |
|  [**SYN\_CCP\_DAQList**](structSYN__CCP__DAQList.md) | [**daq\_lists**](#variable-daq_lists)  <br> |
|  uintptr\_t | [**mta0\_addr**](#variable-mta0_addr)  <br> |
|  uint8\_t | [**mta0\_ext**](#variable-mta0_ext)  <br> |
|  uintptr\_t | [**mta1\_addr**](#variable-mta1_addr)  <br> |
|  uint8\_t | [**mta1\_ext**](#variable-mta1_ext)  <br> |
|  uint8\_t | [**selected\_daq**](#variable-selected_daq)  <br> |
|  uint8\_t | [**selected\_odt**](#variable-selected_odt)  <br> |
|  uint16\_t | [**station\_addr**](#variable-station_addr)  <br> |
|  uint8\_t | [**unlocked\_resources**](#variable-unlocked_resources)  <br> |












































## Public Attributes Documentation




### variable active\_cal\_page 

```C++
uint8_t SYN_CCP_Slave::active_cal_page;
```



Currently active calibration page 


        

<hr>



### variable connected 

```C++
bool SYN_CCP_Slave::connected;
```



Session connection state 


        

<hr>



### variable daq\_lists 

```C++
SYN_CCP_DAQList SYN_CCP_Slave::daq_lists[SYN_CCP_MAX_DAQ_LISTS];
```



Configured DAQ lists 


        

<hr>



### variable mta0\_addr 

```C++
uintptr_t SYN_CCP_Slave::mta0_addr;
```



MTA0 target address 


        

<hr>



### variable mta0\_ext 

```C++
uint8_t SYN_CCP_Slave::mta0_ext;
```



MTA0 extension address 


        

<hr>



### variable mta1\_addr 

```C++
uintptr_t SYN_CCP_Slave::mta1_addr;
```



MTA1 target address 


        

<hr>



### variable mta1\_ext 

```C++
uint8_t SYN_CCP_Slave::mta1_ext;
```



MTA1 extension address 


        

<hr>



### variable selected\_daq 

```C++
uint8_t SYN_CCP_Slave::selected_daq;
```



Currently selected DAQ list index 


        

<hr>



### variable selected\_odt 

```C++
uint8_t SYN_CCP_Slave::selected_odt;
```



Currently selected ODT index 


        

<hr>



### variable station\_addr 

```C++
uint16_t SYN_CCP_Slave::station_addr;
```



CCP station address 


        

<hr>



### variable unlocked\_resources 

```C++
uint8_t SYN_CCP_Slave::unlocked_resources;
```



Bitmask of unlocked resources 


        

<hr>

------------------------------
The documentation for this class was generated from the following file `src/syntropic/proto/syn_ccp.h`

