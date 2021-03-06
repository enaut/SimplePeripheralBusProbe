/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name: 

    internal.h

Abstract:

    This module contains the common internal type and function
    definitions for the SPB controller driver.

Environment:

    kernel-mode only

Revision History:

--*/

#ifndef _INTERNAL_H_
#define _INTERNAL_H_

#pragma warning(push)
#pragma warning(disable:4512)
#pragma warning(disable:4480)

#define SI2C_POOL_TAG ((ULONG) 'C2IS')

/////////////////////////////////////////////////
//
// Common includes.
//
/////////////////////////////////////////////////

#include <initguid.h>
#include <ntddk.h>
#include <wdm.h>
#include <wdf.h>
#include <ntstrsafe.h>

#include "SPBCx.h"
#include "i2ctrace.h"

#define RESHUB_USE_HELPER_ROUTINES
#include "reshub.h"


/////////////////////////////////////////////////
//
// Hardware definitions.
//
/////////////////////////////////////////////////

/////////////////////////////////////////////////
//
// Resource and descriptor definitions.
//
/////////////////////////////////////////////////

#include "reshub.h"

//
// I2C Serial peripheral bus descriptor
//

#include "pshpack1.h"

typedef struct _PNP_I2C_SERIAL_BUS_DESCRIPTOR {
    PNP_SERIAL_BUS_DESCRIPTOR SerialBusDescriptor;
    ULONG ConnectionSpeed;
    USHORT SlaveAddress;
    // follwed by optional Vendor Data
    // followed by PNP_IO_DESCRIPTOR_RESOURCE_NAME
} PNP_I2C_SERIAL_BUS_DESCRIPTOR, *PPNP_I2C_SERIAL_BUS_DESCRIPTOR;

typedef struct _PNP_SPI_SERIAL_BUS_DESCRIPTOR {
	PNP_SERIAL_BUS_DESCRIPTOR SerialBusDescriptor;
	// see ACPI spec
	ULONG ConnectionSpeed;
	UCHAR DataBitLength;
	UCHAR Phase;
	UCHAR Polarity;
	USHORT DeviceSelection;
	// follwed by optional Vendor Data
	// followed by PNP_IO_DESCRIPTOR_RESOURCE_NAME
} PNP_SPI_SERIAL_BUS_DESCRIPTOR, *PPNP_SPI_SERIAL_BUS_DESCRIPTOR;

#include "poppack.h"

#define I2C_SERIAL_BUS_TYPE 0x01
#define I2C_SERIAL_BUS_SPECIFIC_FLAG_10BIT_ADDRESS 0x0001

#define SPI_SERIAL_BUS_TYPE 0x02

/////////////////////////////////////////////////
//
// Settings.
//
/////////////////////////////////////////////////

//
// Power settings.
//

#define IDLE_TIMEOUT_MONITOR_ON  2000
#define IDLE_TIMEOUT_MONITOR_OFF 50

//
// Target settings.
//

typedef enum ADDRESS_MODE
{
    AddressMode7Bit,
    AddressMode10Bit
}
ADDRESS_MODE, *PADDRESS_MODE;

typedef struct PBC_TARGET_SETTINGS
{
    ADDRESS_MODE                  AddressMode;
    USHORT                        Address;
    ULONG                         ConnectionSpeed;
}
PBC_TARGET_SETTINGS, *PPBC_TARGET_SETTINGS;

/////////////////////////////////////////////////
//
// Context definitions.
//
/////////////////////////////////////////////////

typedef struct PBC_DEVICE   PBC_DEVICE,   *PPBC_DEVICE;
typedef struct PBC_TARGET   PBC_TARGET,   *PPBC_TARGET;
typedef struct PBC_REQUEST  PBC_REQUEST,  *PPBC_REQUEST;

//
// Device context.
//

struct PBC_DEVICE 
{
    // Handle to the WDF device.
    WDFDEVICE                      FxDevice;

	//
	// Connection ID for SPB peripheral
	//

	LARGE_INTEGER PeripheralId;
	
	//
	// SPB controller target
	//

	WDFIOTARGET TrueSpbController;

	//
	// SPB request object
	//

	WDFREQUEST SpbRequest;

	//
	// Input memory for request. Valid while request in progress.
	//

	WDFMEMORY InputMemory;

	//
	// Client request object
	//

	SPBREQUEST ClientRequest;

    // Target that the controller is currently
    // configured for. In most cases this value is only
    // set when there is a request being handled, however,
    // it will persist between lock and unlock requests.
    // There cannot be more than one current target.
    PPBC_TARGET                    pCurrentTarget;
    
    // The power setting callback handle
    PVOID                          pMonitorPowerSettingHandle;
};

//
// Target context.
//

struct PBC_TARGET 
{
    // Handle to the SPB target.
    SPBTARGET                      SpbTarget;

    // Target specific settings.
    PBC_TARGET_SETTINGS            Settings;
    
    // Current request associated with the 
    // target. This value should only be non-null
    // when this target is the controller's current
    // target.
    PPBC_REQUEST                   pCurrentRequest;
};

//
// Request context.
//

struct PBC_REQUEST
{
	//
	// Associated framework device object
	//

	WDFDEVICE FxDevice;
	
	//
    // Variables that persist for the lifetime of
    // the request. Specifically these apply to an
    // entire sequence request (not just a single transfer).
    //

    // Handle to the SPB request.
    SPBREQUEST                     SpbRequest;

};

//
// Declate contexts for device, target, and request.
//

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(PBC_DEVICE,  GetDeviceContext);
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(PBC_TARGET,  GetTargetContext);
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(PBC_REQUEST, GetRequestContext);

#pragma warning(pop)

#endif // _INTERNAL_H_
