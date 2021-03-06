/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name: 

    peripheral.cpp

Abstract:

    This module contains the function for interaction 
    with the SPB API.

Environment:

    kernel-mode only

Revision History:

--*/

#include "internal.h"
#include "peripheral.h"

#include "peripheral.tmh"

NTSTATUS
SpbPeripheralOpen(
    _In_  PPBC_DEVICE       pDevice
    )
/*++
 
  Routine Description:

    This routine opens a handle to the SPB controller.

  Arguments:

    pDevice - a pointer to the device context

  Return Value:

    Status

--*/
{
    FuncEntry(TRACE_FLAG_SPBAPI);

    WDF_IO_TARGET_OPEN_PARAMS  openParams;
    NTSTATUS status;

    //
    // Create the device path using the connection ID.
    //

    DECLARE_UNICODE_STRING_SIZE(DevicePath, RESOURCE_HUB_PATH_SIZE);

	WdfDeviceStopIdle(pDevice->FxDevice, WdfTrue);

	if (pDevice->TrueSpbController == WDF_NO_HANDLE)
	{
		status = STATUS_NOT_SUPPORTED;
		goto exit;
	}

	RESOURCE_HUB_CREATE_PATH_FROM_ID(
        &DevicePath,
        pDevice->PeripheralId.LowPart,
        pDevice->PeripheralId.HighPart);

    Trace(
        TRACE_LEVEL_INFORMATION,
		TRACE_FLAG_SPBDDI,
        "Opening handle to SPB target via %wZ",
        &DevicePath);

    //
    // Open a handle to the SPB controller.
    //

    WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(
        &openParams,
        &DevicePath,
        (GENERIC_READ | GENERIC_WRITE));
    
    openParams.ShareAccess = 0;
    openParams.CreateDisposition = FILE_OPEN;
    openParams.FileAttributes = FILE_ATTRIBUTE_NORMAL;
    
    status = WdfIoTargetOpen(
        pDevice->TrueSpbController,
        &openParams);
     
    if (!NT_SUCCESS(status)) 
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_SPBAPI,
            "Failed to open SPB target - %!STATUS!",
            status);
    }

exit:
    FuncExit(TRACE_FLAG_SPBDDI);

    return status;
}

NTSTATUS
SpbPeripheralClose(
    _In_  PPBC_DEVICE       pDevice
    )
/*++
 
  Routine Description:

    This routine closes a handle to the SPB controller.

  Arguments:

    pDevice - a pointer to the device context

  Return Value:

    Status

--*/
{
    FuncEntry(TRACE_FLAG_SPBAPI);

	UNREFERENCED_PARAMETER(pDevice);

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_FLAG_SPBAPI,
        "Closing handle to SPB target");

	if (pDevice->TrueSpbController != WDF_NO_HANDLE)
		WdfIoTargetClose(pDevice->TrueSpbController);

	WdfDeviceResumeIdle(pDevice->FxDevice);

    FuncExit(TRACE_FLAG_SPBAPI);
    
    return STATUS_SUCCESS;
}

VOID
SpbPeripheralLock(
    _In_  PPBC_DEVICE       pDevice,
    _In_  SPBREQUEST        spbRequest
    )
/*++
 
  Routine Description:

    This routine sends a lock command to the SPB controller.

  Arguments:

    pDevice - a pointer to the device context
    spbRequest - the framework request object

  Return Value:

    None

--*/
{
    FuncEntry(TRACE_FLAG_SPBAPI);

    UNREFERENCED_PARAMETER(spbRequest);

    NTSTATUS status;

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_FLAG_SPBAPI,
        "Formatting SPB request %p for IOCTL_SPB_LOCK_CONTROLLER",
        pDevice->SpbRequest);
        
    //
    // Save the client request.
    //

    pDevice->ClientRequest = spbRequest;

    //
    // Initialize the SPB request for lock and send.
    //

    status = WdfIoTargetFormatRequestForIoctl(
        pDevice->TrueSpbController,
        pDevice->SpbRequest,
        IOCTL_SPB_LOCK_CONTROLLER,
        nullptr,
        nullptr,
        nullptr,
        nullptr);

    if (NT_SUCCESS(status))
    {
        status = SpbPeripheralSendRequest(
            pDevice,
            pDevice->SpbRequest,
            spbRequest);
    }

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_SPBAPI,
            "Failed to send SPB request %p for "
            "IOCTL_SPB_LOCK_CONTROLLER - %!STATUS!",
            pDevice->SpbRequest,
            status);

        SpbPeripheralCompleteRequestPair(
            pDevice,
            status,
            0);
    }

    FuncExit(TRACE_FLAG_SPBAPI);
}

VOID
SpbPeripheralUnlock(
    _In_  PPBC_DEVICE       pDevice,
    _In_   SPBREQUEST       spbRequest
    )
/*++
 
  Routine Description:

    This routine sends an unlock command to the SPB controller.

  Arguments:

    pDevice - a pointer to the device context
    spbRequest - the framework request object

  Return Value:

    None

--*/
{
    FuncEntry(TRACE_FLAG_SPBAPI);

    UNREFERENCED_PARAMETER(spbRequest);

    NTSTATUS status;

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_FLAG_SPBAPI,
        "Formatting SPB request %p for IOCTL_SPB_UNLOCK_CONTROLLER",
        pDevice->SpbRequest);
        
    //
    // Save the client request.
    //

    pDevice->ClientRequest = spbRequest;

    //
    // Initialize the SPB request for unlock and send.
    //

    status = WdfIoTargetFormatRequestForIoctl(
        pDevice->TrueSpbController,
        pDevice->SpbRequest,
        IOCTL_SPB_UNLOCK_CONTROLLER,
        nullptr,
        nullptr,
        nullptr,
        nullptr);

    if (NT_SUCCESS(status))
    {
        status = SpbPeripheralSendRequest(
            pDevice,
            pDevice->SpbRequest,
            spbRequest);
    }

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_SPBAPI,
            "Failed to send SPB request %p for "
            "IOCTL_SPB_UNLOCK_CONTROLLER - %!STATUS!",
            pDevice->SpbRequest,
            status);

        SpbPeripheralCompleteRequestPair(
            pDevice,
            status,
            0);
    }

    FuncExit(TRACE_FLAG_SPBAPI);
}

VOID
SpbPeripheralLockConnection(
    _In_  PPBC_DEVICE       pDevice,
    _In_   SPBREQUEST       spbRequest
    )
/*++
 
  Routine Description:

    This routine sends a lock connection command to the SPB controller.

  Arguments:

    pDevice - a pointer to the device context
    spbRequest - the framework request object

  Return Value:

    None

--*/
{
    FuncEntry(TRACE_FLAG_SPBAPI);

    UNREFERENCED_PARAMETER(spbRequest);

    NTSTATUS status;

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_FLAG_SPBAPI,
        "Formatting SPB request %p for IOCTL_SPB_LOCK_CONNECTION",
        pDevice->SpbRequest);
        
    //
    // Save the client request.
    //

    pDevice->ClientRequest = spbRequest;

    //
    // Initialize the SPB request for lock and send.
    //

    status = WdfIoTargetFormatRequestForIoctl(
        pDevice->TrueSpbController,
        pDevice->SpbRequest,
        IOCTL_SPB_LOCK_CONNECTION,
        nullptr,
        nullptr,
        nullptr,
        nullptr);

    if (NT_SUCCESS(status))
    {
        status = SpbPeripheralSendRequest(
            pDevice,
            pDevice->SpbRequest,
            spbRequest);
    }

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_SPBAPI,
            "Failed to send SPB request %p for "
            "IOCTL_SPB_LOCK_CONNECTION - %!STATUS!",
            pDevice->SpbRequest,
            status);

        SpbPeripheralCompleteRequestPair(
            pDevice,
            status,
            0);
    }

    FuncExit(TRACE_FLAG_SPBAPI);
}

VOID
SpbPeripheralUnlockConnection(
    _In_  PPBC_DEVICE       pDevice,
    _In_   SPBREQUEST       spbRequest
    )
/*++
 
  Routine Description:

    This routine sends an unlock connection command to the SPB controller.

  Arguments:

    pDevice - a pointer to the device context
    spbRequest - the framework request object

  Return Value:

    None

--*/
{
    FuncEntry(TRACE_FLAG_SPBAPI);

    UNREFERENCED_PARAMETER(spbRequest);

    NTSTATUS status;

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_FLAG_SPBAPI,
        "Formatting SPB request %p for IOCTL_SPB_UNLOCK_CONNECTION",
        pDevice->SpbRequest);
        
    //
    // Save the client request.
    //

    pDevice->ClientRequest = spbRequest;

    //
    // Initialize the SPB request for unlock and send.
    //

    status = WdfIoTargetFormatRequestForIoctl(
        pDevice->TrueSpbController,
        pDevice->SpbRequest,
        IOCTL_SPB_UNLOCK_CONNECTION,
        nullptr,
        nullptr,
        nullptr,
        nullptr);

    if (NT_SUCCESS(status))
    {
        status = SpbPeripheralSendRequest(
            pDevice,
            pDevice->SpbRequest,
            spbRequest);
    }

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_SPBAPI,
            "Failed to send SPB request %p for "
            "IOCTL_SPB_UNLOCK_CONNECTION - %!STATUS!",
            pDevice->SpbRequest,
            status);

        SpbPeripheralCompleteRequestPair(
            pDevice,
            status,
            0);
    }

    FuncExit(TRACE_FLAG_SPBAPI);
}

VOID
SpbTraceBufferIndex(
	_In_ PPBC_DEVICE pDevice,
	_In_ SPBREQUEST  clientRequest,
	_In_ ULONG       index
)
{
	SPB_TRANSFER_DESCRIPTOR transferDescriptor;
	PMDL pMdl;
	const ULONG max_len = 1024;
	UCHAR pBuffer[max_len] = { 0 };
	CHAR pPrefix[32]; /*  format "device NNN: ##nn write llll -" */
	CHAR pDataString[5 + 3 * 16 + 1]; /* format "0000: XX XX XX XX" */
	int dataIndex;
	SPB_TRANSFER_DESCRIPTOR_INIT(&transferDescriptor);

	SpbRequestGetTransferParameters(
		clientRequest,
		index,
		&transferDescriptor,
		&pMdl);

	for (ULONG offset = 0; offset < (ULONG)transferDescriptor.TransferLength; offset += max_len)
	{
		ULONG length = min((ULONG)transferDescriptor.TransferLength - offset, max_len);

		for (ULONG i = 0; i < length; i++)
		{
			RequestGetByte(pMdl, transferDescriptor.TransferLength, i + offset, &pBuffer[i]);
		}

		sprintf_s(pPrefix, sizeof(pPrefix),
			"device %3I64d: %c#%02d %5s %4lu - ",
			pDevice->PeripheralId.QuadPart,
			index == 0 ? '#' : ' ',
			index,
			transferDescriptor.Direction == SpbTransferDirectionToDevice ? "write" : "read",
			(unsigned long)transferDescriptor.TransferLength
		);

		dataIndex = 0;
		dataIndex += sprintf(&pDataString[dataIndex], "%04x: %02x", offset, pBuffer[0]);
		for (ULONG i = 1; i <= length; i++)
		{
			if ((i % 16 == 0) || i == length)
			{
				Trace(
					TRACE_LEVEL_ERROR,
					TRACE_FLAG_SPBAPI,
					"%s %s",
					pPrefix,
					pDataString
				);
				if (i == length)
				{
					break;
				}
				dataIndex = 0;
				dataIndex += sprintf(&pDataString[dataIndex], "%04x:", i + offset);
			}
			dataIndex += sprintf(&pDataString[dataIndex], " %02x", pBuffer[i]);
		}
	}
}

VOID
SpbTraceBuffers(
	_In_ PPBC_DEVICE pDevice,
	_In_ SPBREQUEST clientRequest
)
{
	SPB_REQUEST_PARAMETERS parameters;
	
	SPB_REQUEST_PARAMETERS_INIT(&parameters);

	SpbRequestGetParameters(clientRequest, &parameters);

	for (ULONG i = 0; i < parameters.SequenceTransferCount; i += 1)
	{
		SpbTraceBufferIndex(pDevice, clientRequest, i);
	}

}

VOID
SpbPeripheralRead(
    _In_  PPBC_DEVICE       pDevice,
    _In_   SPBREQUEST       spbRequest,
	_In_  BOOLEAN           FullDuplex
    )
/*++
 
  Routine Description:

    This routine reads from the SPB controller.

  Arguments:

    pDevice - a pointer to the device context
    spbRequest - the framework request object

  Return Value:

    None

--*/
{
    FuncEntry(TRACE_FLAG_SPBAPI);

    UNREFERENCED_PARAMETER(spbRequest);

	WDFMEMORY memory = nullptr;
    NTSTATUS status;

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_FLAG_SPBAPI,
        "Formatting SPB request %p for read",
        pDevice->SpbRequest);
        
    //
    // Save the client request.
    //

    pDevice->ClientRequest = spbRequest;

    //
    // Initialize the SPB request for read and send.
    //

	if (!FullDuplex)
	{
		status = WdfRequestRetrieveOutputMemory(
			spbRequest,
			&memory);
	}
	else
	{
		status = WdfRequestRetrieveInputMemory(
			spbRequest,
			&memory);
	}

	if (NT_SUCCESS(status))
    {
		status = WdfIoTargetFormatRequestForRead(
			pDevice->TrueSpbController,
			pDevice->SpbRequest,
			memory,
			nullptr,
			nullptr);

        if (NT_SUCCESS(status))
        {
            status = SpbPeripheralSendRequest(
                pDevice,
                pDevice->SpbRequest,
                spbRequest);
        }
    }

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_SPBAPI,
            "Failed to send SPB request %p for "
            "read - %!STATUS!",
            pDevice->SpbRequest,
            status);

        SpbPeripheralCompleteRequestPair(
            pDevice,
            status,
            0);
    }

    FuncExit(TRACE_FLAG_SPBAPI);
}

VOID
SpbPeripheralWrite(
    _In_  PPBC_DEVICE       pDevice,
    _In_   SPBREQUEST       spbRequest,
	_In_  BOOLEAN           FullDuplex
    )
/*++
 
  Routine Description:

    This routine writes to the SPB controller.

  Arguments:

    pDevice - a pointer to the device context
    spbRequest - the framework request object

  Return Value:

    None

--*/
{
    FuncEntry(TRACE_FLAG_SPBAPI);

    UNREFERENCED_PARAMETER(spbRequest);

    WDFMEMORY memory = nullptr;
	NTSTATUS status;

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_FLAG_SPBAPI,
        "Formatting SPB request %p for write",
        pDevice->SpbRequest);
        
    //
    // Save the client request.
    //

    pDevice->ClientRequest = spbRequest;

	//
    // Initialize the SPB request for write and send.
    //

	if (!FullDuplex)
	{
		status = WdfRequestRetrieveInputMemory(
			spbRequest,
			&memory);
	}
	else
	{
		status = WdfRequestRetrieveOutputMemory(
			spbRequest,
			&memory);
	}

    if (NT_SUCCESS(status))
    {
        status = WdfIoTargetFormatRequestForWrite(
            pDevice->TrueSpbController,
            pDevice->SpbRequest,
            memory,
            nullptr,
            nullptr);

        if (NT_SUCCESS(status))
        {
            status = SpbPeripheralSendRequest(
                pDevice,
                pDevice->SpbRequest,
                spbRequest);
        }
    }

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_SPBAPI,
            "Failed to send SPB request %p for "
            "write - %!STATUS!",
            pDevice->SpbRequest,
            status);

        SpbPeripheralCompleteRequestPair(
            pDevice,
            status,
            0);
    }

    FuncExit(TRACE_FLAG_SPBAPI);
}

VOID
SpbPeripheralFullDuplex(
	_In_  PPBC_DEVICE       pDevice,
	_In_  SPBREQUEST        spbRequest
)
/*++

Routine Description:

This routine sends a full duplex transfer to the SPB controller.

Arguments:

pDevice - a pointer to the device context
spbRequest - the framework request object

Return Value:

None

--*/
{
	FuncEntry(TRACE_FLAG_SPBAPI);

	UNREFERENCED_PARAMETER(spbRequest);

	WDF_OBJECT_ATTRIBUTES attributes;
	PPBC_REQUEST pRequest;
	NTSTATUS status;

	pRequest = GetRequestContext(pDevice->SpbRequest);

	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_FLAG_SPBAPI,
		"Formatting SPB request %p for IOCTL_SPB_FULL_DUPLEX",
		pDevice->SpbRequest);

	//
	// Save the client request.
	//

	pDevice->ClientRequest = spbRequest;

	//
	// Get input and output buffers.
	//

	const ULONG fullDuplexWriteIndex = 0;
	const ULONG fullDuplexReadIndex = 1;

	SPB_TRANSFER_DESCRIPTOR writeDescriptor;
	SPB_TRANSFER_DESCRIPTOR readDescriptor;
	PMDL pWriteMdl;
	PMDL pReadMdl;

	SPB_TRANSFER_DESCRIPTOR_INIT(&writeDescriptor);
	SPB_TRANSFER_DESCRIPTOR_INIT(&readDescriptor);

	SpbRequestGetTransferParameters(
		spbRequest,
		fullDuplexWriteIndex,
		&writeDescriptor,
		&pWriteMdl);

	SpbRequestGetTransferParameters(
		spbRequest,
		fullDuplexReadIndex,
		&readDescriptor,
		&pReadMdl);

	//
	// Build full duplex transfer using SPB transfer list.
	//

	const ULONG transfers = 2;

	SPB_TRANSFER_LIST_AND_ENTRIES(transfers) seq;
	SPB_TRANSFER_LIST_INIT(&(seq.List), transfers);

	{
		//
		// PreFAST cannot figure out the SPB_TRANSFER_LIST_ENTRY
		// "struct hack" size but using an index variable quiets 
		// the warning. This is a false positive from OACR.
		// 

		const ULONG index = 0;

		seq.List.Transfers[index] = SPB_TRANSFER_LIST_ENTRY_INIT_MDL(
			SpbTransferDirectionToDevice,
			0,
			pWriteMdl);

		seq.List.Transfers[index + 1] = SPB_TRANSFER_LIST_ENTRY_INIT_MDL(
			SpbTransferDirectionFromDevice,
			0,
			pReadMdl);
	}

	//
	// Create preallocated WDFMEMORY. The IOCTL is METHOD_BUFFERED,
	// so the memory doesn't have to persist until the request is
	// completed.
	//

	NT_ASSERT(pDevice->InputMemory == WDF_NO_HANDLE);

	WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

	status = WdfMemoryCreatePreallocated(
		&attributes,
		(PVOID)&seq,
		sizeof(seq),
		&pDevice->InputMemory);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_FLAG_SPBAPI,
			"Failed to create WDFMEMORY - %!STATUS!",
			status);

		goto Done;
	}

	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_FLAG_SPBAPI,
		"Built full duplex transfer %p with byte length=%lu",
		&seq,
		(ULONG)(writeDescriptor.TransferLength + readDescriptor.TransferLength));

	//
	// Send full duplex IOCTL.
	//

	//
	// Format and send the full duplex request.
	//

	status = WdfIoTargetFormatRequestForIoctl(
		pDevice->TrueSpbController,
		pDevice->SpbRequest,
		IOCTL_SPB_FULL_DUPLEX,
		pDevice->InputMemory,
		nullptr,
		nullptr,
		nullptr);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_FLAG_SPBAPI,
			"Failed to format request - %!STATUS!",
			status);

		goto Done;
	}

	status = SpbPeripheralSendRequest(
		pDevice,
		pDevice->SpbRequest,
		spbRequest);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_FLAG_SPBAPI,
			"Failed to send SPB request %p for "
			"IOCTL_SPB_FULL_DUPLEX - %!STATUS!",
			pDevice->SpbRequest,
			status);

		goto Done;
	}

Done:

	if (!NT_SUCCESS(status))
	{
		SpbPeripheralCompleteRequestPair(
			pDevice,
			status,
			0);
	}

	FuncExit(TRACE_FLAG_SPBAPI);
}

NTSTATUS
SpbPeripheralSequence1(
	_In_  PPBC_DEVICE       pDevice,
	_In_  SPBREQUEST        spbRequest
)
/*++

Routine Description:

This routine sends a sequence of 1 transfer to the SPB controller.

Arguments:

pDevice - a pointer to the device context
spbRequest - the framework request object

Return Value:

None

--*/
{
	FuncEntry(TRACE_FLAG_SPBAPI);

	UNREFERENCED_PARAMETER(spbRequest);

	WDF_OBJECT_ATTRIBUTES attributes;
	PPBC_REQUEST pRequest;
	NTSTATUS status;

	pRequest = GetRequestContext(pDevice->SpbRequest);

	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_FLAG_SPBAPI,
		"Formatting SPB request %p for IOCTL_SPB_EXECUTE_SEQUENCE",
		pDevice->SpbRequest);

	//
	// Save the client request.
	//

	pDevice->ClientRequest = spbRequest;

	//
	// Get buffer.
	//

	SPB_TRANSFER_DESCRIPTOR firstDescriptor;
	PMDL pFirstMdl;

	SPB_TRANSFER_DESCRIPTOR_INIT(&firstDescriptor);

	SpbRequestGetTransferParameters(
		spbRequest,
		0,
		&firstDescriptor,
		&pFirstMdl);

	//
	// Build full duplex transfer using SPB transfer list.
	//

	const ULONG transfers = 2;

	SPB_TRANSFER_LIST seq;
	SPB_TRANSFER_LIST_INIT(&seq, 1);

	{
		//
		// PreFAST cannot figure out the SPB_TRANSFER_LIST_ENTRY
		// "struct hack" size but using an index variable quiets 
		// the warning. This is a false positive from OACR.
		// 

		const ULONG index = 0;

		seq.Transfers[index] = SPB_TRANSFER_LIST_ENTRY_INIT_MDL(
			firstDescriptor.Direction,
			0,
			pFirstMdl);
	}

	//
	// Create preallocated WDFMEMORY. The IOCTL is METHOD_BUFFERED,
	// so the memory doesn't have to persist until the request is
	// completed.
	//

	NT_ASSERT(pDevice->InputMemory == WDF_NO_HANDLE);

	WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

	status = WdfMemoryCreatePreallocated(
		&attributes,
		(PVOID)&seq,
		sizeof(seq),
		&pDevice->InputMemory);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_FLAG_SPBAPI,
			"Failed to create WDFMEMORY - %!STATUS!",
			status);

		goto Done;
	}

	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_FLAG_SPBAPI,
		"Built sequence transfer %p with byte length=%lu",
		&seq,
		(ULONG)(firstDescriptor.TransferLength));

	//
	// Send full duplex IOCTL.
	//

	//
	// Format and send the full duplex request.
	//

	status = WdfIoTargetFormatRequestForIoctl(
		pDevice->TrueSpbController,
		pDevice->SpbRequest,
		IOCTL_SPB_EXECUTE_SEQUENCE,
		pDevice->InputMemory,
		nullptr,
		nullptr,
		nullptr);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_FLAG_SPBAPI,
			"Failed to format request - %!STATUS!",
			status);

		goto Done;
	}

	status = SpbPeripheralSendRequest(
		pDevice,
		pDevice->SpbRequest,
		spbRequest);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_FLAG_SPBAPI,
			"Failed to send SPB request %p for "
			"IOCTL_SPB_EXECUTE_SEQUENCE - %!STATUS!",
			pDevice->SpbRequest,
			status);

		goto Done;
	}

Done:

	FuncExit(TRACE_FLAG_SPBAPI);

	return status;
}

NTSTATUS
SpbPeripheralSequence2(
	_In_  PPBC_DEVICE       pDevice,
	_In_  SPBREQUEST        spbRequest
)
/*++

Routine Description:

This routine sends a sequence of 2 transfers to the SPB controller.

Arguments:

pDevice - a pointer to the device context
spbRequest - the framework request object

Return Value:

None

--*/
{
	FuncEntry(TRACE_FLAG_SPBAPI);

	UNREFERENCED_PARAMETER(spbRequest);

	WDF_OBJECT_ATTRIBUTES attributes;
	PPBC_REQUEST pRequest;
	NTSTATUS status;

	pRequest = GetRequestContext(pDevice->SpbRequest);

	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_FLAG_SPBAPI,
		"Formatting SPB request %p for IOCTL_SPB_EXECUTE_SEQUENCE",
		pDevice->SpbRequest);

	//
	// Save the client request.
	//

	pDevice->ClientRequest = spbRequest;

	//
	// Get input and output buffers.
	//

	SPB_TRANSFER_DESCRIPTOR firstDescriptor;
	SPB_TRANSFER_DESCRIPTOR secondDescriptor;
	PMDL pFirstMdl;
	PMDL pSecondMdl;

	SPB_TRANSFER_DESCRIPTOR_INIT(&firstDescriptor);
	SPB_TRANSFER_DESCRIPTOR_INIT(&secondDescriptor);

	SpbRequestGetTransferParameters(
		spbRequest,
		0,
		&firstDescriptor,
		&pFirstMdl);

	SpbRequestGetTransferParameters(
		spbRequest,
		1,
		&secondDescriptor,
		&pSecondMdl);

	//
	// Build full duplex transfer using SPB transfer list.
	//

	const ULONG transfers = 2;

	SPB_TRANSFER_LIST_AND_ENTRIES(transfers) seq;
	SPB_TRANSFER_LIST_INIT(&(seq.List), transfers);

	{
		//
		// PreFAST cannot figure out the SPB_TRANSFER_LIST_ENTRY
		// "struct hack" size but using an index variable quiets 
		// the warning. This is a false positive from OACR.
		// 

		const ULONG index = 0;

		seq.List.Transfers[index] = SPB_TRANSFER_LIST_ENTRY_INIT_MDL(
			firstDescriptor.Direction,
			0,
			pFirstMdl);

		seq.List.Transfers[index + 1] = SPB_TRANSFER_LIST_ENTRY_INIT_MDL(
			secondDescriptor.Direction,
			0,
			pSecondMdl);
	}

	//
	// Create preallocated WDFMEMORY. The IOCTL is METHOD_BUFFERED,
	// so the memory doesn't have to persist until the request is
	// completed.
	//

	NT_ASSERT(pDevice->InputMemory == WDF_NO_HANDLE);

	WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

	status = WdfMemoryCreatePreallocated(
		&attributes,
		(PVOID)&seq,
		sizeof(seq),
		&pDevice->InputMemory);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_FLAG_SPBAPI,
			"Failed to create WDFMEMORY - %!STATUS!",
			status);

		goto Done;
	}

	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_FLAG_SPBAPI,
		"Built sequence transfer %p with byte length=%lu",
		&seq,
		(ULONG)(firstDescriptor.TransferLength + secondDescriptor.TransferLength));

	//
	// Send full duplex IOCTL.
	//

	//
	// Format and send the full duplex request.
	//

	status = WdfIoTargetFormatRequestForIoctl(
		pDevice->TrueSpbController,
		pDevice->SpbRequest,
		IOCTL_SPB_EXECUTE_SEQUENCE,
		pDevice->InputMemory,
		nullptr,
		nullptr,
		nullptr);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_FLAG_SPBAPI,
			"Failed to format request - %!STATUS!",
			status);

		goto Done;
	}

	status = SpbPeripheralSendRequest(
		pDevice,
		pDevice->SpbRequest,
		spbRequest);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_FLAG_SPBAPI,
			"Failed to send SPB request %p for "
			"IOCTL_SPB_EXECUTE_SEQUENCE - %!STATUS!",
			pDevice->SpbRequest,
			status);

		goto Done;
	}

Done:

	FuncExit(TRACE_FLAG_SPBAPI);

	return status;
}

VOID
SpbPeripheralSequence(
	_In_  PPBC_DEVICE       pDevice,
	_In_  SPBREQUEST        spbRequest,
	_In_  ULONG             TransferCount
)
/*++

Routine Description:

This routine sends a sequence of 2 transfers to the SPB controller.

Arguments:

pDevice - a pointer to the device context
spbRequest - the framework request object
TransferCount - the transfer count

Return Value:

None

--*/
{
	FuncEntry(TRACE_FLAG_SPBAPI);

	NTSTATUS status = STATUS_NOT_SUPPORTED;

	switch (TransferCount)
	{
	case 1:
		status = SpbPeripheralSequence1(pDevice, spbRequest);
		break;
	case 2:
		status = SpbPeripheralSequence2(pDevice, spbRequest);
		break;
	}

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_FLAG_SPBAPI,
			"Failed to send SPB request %p for "
			"IOCTL_SPB_FULL_DUPLEX - %!STATUS!",
			pDevice->SpbRequest,
			status);

		goto Done;
	}

Done:

	if (!NT_SUCCESS(status))
	{
		SpbPeripheralCompleteRequestPair(
			pDevice,
			status,
			0);
	}

	FuncExit(TRACE_FLAG_SPBAPI);
}

NTSTATUS
SpbPeripheralSendRequest(
    _In_  PPBC_DEVICE       pDevice,
    _In_  WDFREQUEST       SpbRequest,
    _In_  WDFREQUEST       ClientRequest
    )
/*++
 
  Routine Description:

    This routine sends a write-read sequence to the SPB controller.

  Arguments:

    pDevice - a pointer to the device context
    SpbRequest - the SPB request object
    ClientRequest - the client request object

  Return Value:

    Status

--*/
{
    FuncEntry(TRACE_FLAG_SPBAPI);
    
    PPBC_REQUEST pRequest = GetRequestContext(ClientRequest);
    NTSTATUS status = STATUS_SUCCESS;

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_FLAG_SPBAPI,
        "Saving client request %p, and "
        "sending SPB request %p",
        ClientRequest,
        SpbRequest);

    //
    // Init client request context.
    //

    pRequest->FxDevice = pDevice->FxDevice;

    //
    // Mark the client request as cancellable.
    //

    if (NT_SUCCESS(status))
    {
        status = WdfRequestMarkCancelableEx(
            ClientRequest,
            SpbPeripheralOnCancel);
    }

    //
    // Send the SPB request.
    //

    if (NT_SUCCESS(status))
    {
        WdfRequestSetCompletionRoutine(
            SpbRequest,
            SpbPeripheralOnCompletion,
            GetRequestContext(SpbRequest));

        BOOLEAN fSent = WdfRequestSend(
            SpbRequest,
            pDevice->TrueSpbController,
            WDF_NO_SEND_OPTIONS);

        if (!fSent)
        {
            status = WdfRequestGetStatus(SpbRequest);

            Trace(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_SPBAPI,
                "Failed to send SPB request %p - %!STATUS!",
                SpbRequest,
                status);

            NTSTATUS cancelStatus;
            cancelStatus = WdfRequestUnmarkCancelable(ClientRequest);

            if (!NT_SUCCESS(cancelStatus))
            {
                NT_ASSERTMSG("WdfRequestUnmarkCancelable should only fail if request has already been cancelled",
                    cancelStatus == STATUS_CANCELLED);

                Trace(
                    TRACE_LEVEL_INFORMATION, 
                    TRACE_FLAG_SPBAPI, 
                    "Client request %p has already been cancelled - "
                    "%!STATUS!",
                    ClientRequest,
                    cancelStatus);
            }
        }
    }

    FuncExit(TRACE_FLAG_SPBAPI);

    return status;
}

VOID 
SpbPeripheralOnCompletion(
    _In_  WDFREQUEST                      spbRequest,
    _In_  WDFIOTARGET                     FxTarget,
    _In_  PWDF_REQUEST_COMPLETION_PARAMS  Params,
    _In_  WDFCONTEXT                      Context
    )
/*++
 
  Routine Description:

    This routine is called when a request completes.

  Arguments:

    spbRequest - the framework request object
    FxTarget - the framework IO target object
    Params - a pointer to the request completion parameters
    Context - the request context

  Return Value:

    None

--*/
{
    FuncEntry(TRACE_FLAG_SPBAPI);
    
    UNREFERENCED_PARAMETER(FxTarget);
    UNREFERENCED_PARAMETER(Context);
    
    PPBC_REQUEST pRequest;
    PPBC_DEVICE pDevice;
    NTSTATUS status;
    NTSTATUS cancelStatus;
    ULONG_PTR bytesCompleted;

    pRequest = GetRequestContext(spbRequest);
    pDevice = GetDeviceContext(pRequest->FxDevice);

    status = Params->IoStatus.Status;

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_FLAG_SPBAPI,
        "Completion callback received for SPB request %p with %!STATUS!",
        spbRequest,
        status);

	//if (NT_SUCCESS(status))
	//{
	//	SPB_TRANSFER_DESCRIPTOR descriptor;
	//	SPB_TRANSFER_DIRECTION direction;
	//	PMDL pMdl;
	//	size_t transferLength;
	//	UCHAR buffer[8];
	//	unsigned int i;

	//	SPB_TRANSFER_DESCRIPTOR_INIT(&descriptor);

	//	SpbRequestGetTransferParameters(
	//		pRequest->SpbRequest,
	//		0,
	//		&descriptor,
	//		&pMdl);

	//	NT_ASSERT(pMdl != NULL);

	//	transferLength = descriptor.TransferLength;
	//	direction = descriptor.Direction;

	//	for (i = 0; i < sizeof(buffer) / sizeof(buffer[0]); i++)
	//	{
	//		RequestGetByte(pMdl, transferLength, i, &buffer[i]);
	//	}

	//	Trace(
	//		TRACE_LEVEL_ERROR,
	//		TRACE_FLAG_SPBAPI,
	//		"buffer %lu bytes: %02x %02x %02x %02x %02x %02x %02x %02x",
	//		(unsigned long)transferLength,
	//		buffer[0],
	//		buffer[1],
	//		buffer[2],
	//		buffer[3],
	//		buffer[4],
	//		buffer[5],
	//		buffer[6],
	//		buffer[7]
	//	);

	//}
	//if (NT_SUCCESS(status))
	//{
	//	NTSTATUS dbg_status;
	//	PVOID pInputBuffer = nullptr;
	//	PVOID pOutputBuffer = nullptr;
	//	size_t inputBufferLength = 0;
	//	size_t outputBufferLength = 0;

	//	dbg_status = WdfRequestRetrieveInputBuffer(
	//		spbRequest,
	//		0,
	//		&pInputBuffer,
	//		&inputBufferLength);

	//	if (NT_SUCCESS(dbg_status))
	//	{
	//		Trace(
	//			TRACE_LEVEL_ERROR,
	//			TRACE_FLAG_SPBAPI,
	//			"Retrieved input buffer"
	//		);
	//	}

	//	status = WdfRequestRetrieveOutputBuffer(
	//		spbRequest,
	//		0,
	//		&pOutputBuffer,
	//		&outputBufferLength);

	//	if (NT_SUCCESS(dbg_status))
	//	{
	//		Trace(
	//			TRACE_LEVEL_ERROR,
	//			TRACE_FLAG_SPBAPI,
	//			"Retrieved output buffer"
	//		);
	//	}
	//}

    //
    // Unmark the client request as cancellable
    //

    cancelStatus = WdfRequestUnmarkCancelable(pDevice->ClientRequest);

    if (!NT_SUCCESS(cancelStatus))
    {
        NT_ASSERTMSG("WdfRequestUnmarkCancelable should only fail if request has already been cancelled",
            cancelStatus == STATUS_CANCELLED);

        Trace(
            TRACE_LEVEL_INFORMATION, 
            TRACE_FLAG_SPBAPI, 
            "Client request %p has already been cancelled - %!STATUS!",
            pDevice->ClientRequest,
            cancelStatus);
    }

    //
    // Complete the request pair
    //

    bytesCompleted = Params->IoStatus.Information;

    SpbPeripheralCompleteRequestPair(
        pDevice,
        status,
        bytesCompleted);
    
    FuncExit(TRACE_FLAG_SPBAPI);
}

VOID
SpbPeripheralOnCancel(
    _In_  WDFREQUEST  spbRequest
    )
/*++
Routine Description:

    This event is called when the client request is cancelled.

Arguments:

    spbRequest - the framework request object

Return Value:

   VOID

--*/
{
    FuncEntry(TRACE_FLAG_SPBAPI);

    PPBC_REQUEST pRequest;
    PPBC_DEVICE pDevice;

    pRequest = GetRequestContext(spbRequest);
    pDevice = GetDeviceContext(pRequest->FxDevice);

    //
    // Attempt to cancel the SPB request
    //
    
    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_FLAG_SPBAPI,
        "Cancel received for client request %p, "
        "attempting to cancel SPB request %p",
        spbRequest,
        pDevice->SpbRequest);

    WdfRequestCancelSentRequest(pDevice->SpbRequest);

    FuncExit(TRACE_FLAG_SPBAPI);
}

VOID
SpbPeripheralCompleteRequestPair(
    _In_  PPBC_DEVICE       pDevice,
    _In_  NTSTATUS         status,
    _In_  ULONG_PTR        bytesCompleted
    )
/*++
Routine Description:

    This routine marks the SpbRequest as reuse
    and completes the client request.

Arguments:

    pDevice - the device context
    status - the client completion status
    bytesCompleted - the number of bytes completed
        for the client request

Return Value:

   VOID

--*/
{
    FuncEntry(TRACE_FLAG_SPBAPI);

    PPBC_REQUEST pRequest;
    pRequest = GetRequestContext(pDevice->SpbRequest);

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_FLAG_SPBAPI,
        "Marking SPB request %p for reuse, and completing "
        "client request %p with %!STATUS! and bytes=%lu",
        pDevice->SpbRequest,
        pDevice->ClientRequest,
        status,
        (ULONG)bytesCompleted);

    //
    // Mark the SPB request as reuse
    //

    WDF_REQUEST_REUSE_PARAMS params;
    WDF_REQUEST_REUSE_PARAMS_INIT(
        &params,
        WDF_REQUEST_REUSE_NO_FLAGS,
        STATUS_SUCCESS);

    WdfRequestReuse(pDevice->SpbRequest, &params);

    if (pDevice->InputMemory != WDF_NO_HANDLE)
    {
        WdfObjectDelete(pDevice->InputMemory);
        pDevice->InputMemory = WDF_NO_HANDLE;
    }

    //
    // Complete the client request
    //

    if (pDevice->ClientRequest != nullptr)
    {
        SPBREQUEST clientRequest = pDevice->ClientRequest;
        pDevice->ClientRequest = nullptr;

		SpbTraceBuffers(pDevice, clientRequest);

        // In order to satisfy SDV, assume clientRequest
        // is equal to pDevice->ClientRequest. This suppresses
        // a warning in the driver's cancellation path. 
        //
        // Typically when WdfRequestUnmarkCancelable returns 
        // STATUS_CANCELLED a driver does not go on to complete 
        // the request in that context. This sample, however, 
        // driver has handled this condition appropriately by 
        // not completing the cancelled request in its 
        // EvtRequestCancel callback. Developers should be 
        // cautious when copying code from this sample, paying 
        // close attention to the cancellation logic.
        //
        _Analysis_assume_(clientRequest == pDevice->ClientRequest);
        
        WdfRequestCompleteWithInformation(
            clientRequest,
            status,
            bytesCompleted);
    }

    FuncExit(TRACE_FLAG_SPBAPI);
}
