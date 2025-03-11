#if defined(CFLAT_ENABLED)

#  include "CflatDebugAdapter.h"

#  include "Sockets.h"
#  include "SocketSubsystem.h"

static const uint8 kMessageEndMarker = 0x04;

static const double kSleepTime = 0.033;
static const double kSocketWaitTime = 0.5;
static const double kConnectionCheckInterval = 0.5;

CflatDebugAdapter::CflatDebugAdapter()
    : mSocketSubsystem(nullptr),
      mListener(nullptr),
      mListening(false),
      mDebugLog(false),
      mListeningAdress("127.0.0.1")
{
}

CflatDebugAdapter::~CflatDebugAdapter()
{
  if (mSocketSubsystem && mSocket)
  {
    mSocketSubsystem->DestroySocket(mSocket);
    mSocket = nullptr;
  }
}

bool CflatDebugAdapter::Start(int32 pPort)
{
  if (!FSlateApplication::IsInitialized())
  {
    return false;
  }
  mPort = pPort;
  mConnectionLastCheck = FPlatformTime::Seconds();
  mSocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
  mListener = mSocketSubsystem->CreateSocket(
      NAME_Stream, TEXT("Cflat Debug Adapater"), false);
  mThread = FRunnableThread::Create(
      this, TEXT("CflatDebuggerConnection"), 0, TPri_BelowNormal);

  Listen();

  return true;
}

void CflatDebugAdapter::AddRequestListener(
    FName pRequestName, EventCallback pCallback)
{
  mRequestCallbacks.Add(pRequestName, pCallback);
}

TSharedPtr<FJsonObject> CflatDebugAdapter::CreateResponse(
    const TSharedPtr<FJsonObject> pRequest, bool pSuccess)
{
  TSharedPtr<FJsonObject> jsonObject = MakeShareable(new FJsonObject);
  jsonObject->SetStringField(TEXT("type"), TEXT("response"));
  jsonObject->SetBoolField(TEXT("success"), pSuccess);
  jsonObject->SetNumberField(TEXT("seq"), 0);

  {
    TSharedPtr<FJsonValue> val = pRequest->TryGetField(TEXT("seq"));
    jsonObject->SetField(TEXT("request_seq"), val);
  }
  {
    TSharedPtr<FJsonValue> val = pRequest->TryGetField(TEXT("command"));
    jsonObject->SetField(TEXT("command"), val);
  }

  return jsonObject;
}

TSharedPtr<FJsonObject> CflatDebugAdapter::CreateEvent(const FString& pName)
{
  TSharedPtr<FJsonObject> jsonObject = MakeShareable(new FJsonObject);
  jsonObject->SetStringField(TEXT("type"), TEXT("event"));
  jsonObject->SetStringField(TEXT("event"), pName);

  return jsonObject;
}

void CflatDebugAdapter::SendEvent(const TSharedPtr<FJsonObject> pEvent)
{
  if (!IsConnected())
  {
    return;
  }

  FString jsonString;
  auto writer =
      TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(
          &jsonString);
  FJsonSerializer::Serialize(pEvent.ToSharedRef(), writer);

  SendString(jsonString);
}

void CflatDebugAdapter::SendResponse(const TSharedPtr<FJsonObject> pResponse)
{
  if (!IsConnected())
  {
    return;
  }

  FString jsonString;
  auto writer =
      TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(
          &jsonString);
  FJsonSerializer::Serialize(pResponse.ToSharedRef(), writer);

  SendString(jsonString);
}

bool CflatDebugAdapter::Listen()
{
  if (!mListening)
  {
    bool addrValid = true;
    bool canBindAll = false;

    TSharedPtr<FInternetAddr> addr = mSocketSubsystem->CreateInternetAddr();
    addr->SetIp(*mListeningAdress, addrValid);
    addr->SetPort(mPort);

    if (!addrValid)
    {
      UE_LOG(
          LogTemp,
          Error,
          TEXT("[CflatDebugAdapter] Socket address not valid: %s"),
          *addr->ToString(true));
      return false;
    }

    if (!mListener->Bind(*addr))
    {
      UE_LOG(
          LogTemp,
          Error,
          TEXT("[CflatDebugAdapter] Could not bind Socket to address: %s"),
          *addr->ToString(true));
      return false;
    }

    mListening = mListener->Listen(0);
    if (mListening)
    {
      UE_LOG(
          LogTemp,
          Log,
          TEXT("[CflatDebugAdapter] Socket listening to: %s"),
          *addr->ToString(true));
    }
    else
    {
      UE_LOG(
          LogTemp,
          Error,
          TEXT("[CflatDebugAdapter] Socket failed to listen to: %s"),
          *addr->ToString(true));
    }
  }
  return mListening;
}

bool CflatDebugAdapter::Disconnect()
{
  if (mSocket && mSocketSubsystem)
  {
    mSocketSubsystem->DestroySocket(mSocket);
    mSocket = nullptr;
    return true;
  }
  return false;
}

bool CflatDebugAdapter::IsConnected()
{
  return mSocket != nullptr && mSocket->GetConnectionState() == SCS_Connected;
}

void CflatDebugAdapter::CheckListener()
{
  if (mConnectionLastCheck + kConnectionCheckInterval >
      FPlatformTime::Seconds())
  {
    return;
  }

  mConnectionLastCheck = FPlatformTime::Seconds();

  if (mListening)
  {
    bool hasPendingConnection = false;
    if (mListener->HasPendingConnection(hasPendingConnection))
    {
      if (hasPendingConnection)
      {

        UE_LOG(
            LogTemp, Log, TEXT("[CflatDebugAdapter] Has Pending Connection!"));

        if (mSocket)
        {
          UE_LOG(
              LogTemp, Log, TEXT("[CflatDebugAdapter] Destroying old Socket"));
          mSocketSubsystem->DestroySocket(mSocket);
        }

        FSocket* incomming = mListener->Accept(TEXT("Request"));
        if (incomming)
        {
          mSocket = incomming;
          UE_LOG(LogTemp, Log, TEXT("[CflatDebugAdapter] Connected!"));
        }
        else
        {
          mSocket = nullptr;

          ESocketErrors errorCode = mSocketSubsystem->GetLastErrorCode();
          FString errorStr = mSocketSubsystem->GetSocketError();

          UE_LOG(
              LogTemp,
              Error,
              TEXT("Error accepting expected connection [%d] %s"),
              (int32)errorCode,
              *errorStr);
        }
      }
    }
  }
}

void CflatDebugAdapter::ParseMessageData(const uint8* pData, int32 pSize)
{
  const char* charData = (const char*)pData;
  FString jsonString(charData, pSize);

  auto jsonReader = TJsonReaderFactory<>::Create(jsonString);

  TSharedPtr<FJsonObject> jsonObject;
  if (!FJsonSerializer::Deserialize(jsonReader, jsonObject))
  {
    UE_LOG(
        LogTemp,
        Error,
        TEXT("[CflatDebugAdapter] Invalid json received: %s"),
        *jsonString);
    return;
  }

  if (mDebugLog)
  {
    UE_LOG(
        LogTemp,
        Log,
        TEXT("[CflatDebugAdapter] -->> Received json(%d): %s"),
        pSize,
        *jsonString);
  }

  FString messageType;
  if (!jsonObject->TryGetStringField(TEXT("type"), messageType))
  {
    UE_LOG(
        LogTemp,
        Error,
        TEXT("[CflatDebugAdapter] Invalid message received: %s"),
        *jsonString);
    return;
  }

  if (messageType == (TEXT("request")))
  {
    FString command;
    if (!jsonObject->TryGetStringField(TEXT("command"), command))
    {
      UE_LOG(
          LogTemp,
          Error,
          TEXT("[CflatDebugAdapter] Request is missing command"),
          *messageType);
      return;
    }

    FName commandName(command);
    auto callback = mRequestCallbacks.Find(commandName);
    if (callback)
    {
      if (mDebugLog)
      {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[CflatDebugAdapter] Calling callback: %s"),
            *commandName.ToString());
      }
      (*callback)(jsonObject);
    }
    else
    {
      if (mDebugLog)
      {
        UE_LOG(
            LogTemp,
            Log,
            TEXT("[CflatDebugAdapter] No Callback registered for command: %s"),
            *command);
      }
      SendResponse(CreateResponse(jsonObject, false));
      return;
    }
  }
}

void CflatDebugAdapter::ReadData()
{
  int32 bytesRead = 0;
  bool success = mSocket->Recv(mIncomingBuffer, kIncomingBufferSize, bytesRead);
  if (success && bytesRead > 0)
  {
    int32 dataBegin = 0;
    for (int32 bi = 0; bi < bytesRead; ++bi)
    {
      if (mIncomingBuffer[bi] == kMessageEndMarker)
      {
        mIncomingBuffer[bi] = 0;
        ParseMessageData(&mIncomingBuffer[dataBegin], bi + 1);
        dataBegin = bi + 1;
      }
    }
  }
  else
  {
    UE_LOG(LogTemp, Error, TEXT("[CflatDebugAdapter] Error Receiving data"));
  }
}

void CflatDebugAdapter::SendString(const FString& pString)
{
  if (!IsConnected())
  {
    UE_LOG(
        LogTemp,
        Error,
        TEXT("[CflatDebugAdapter] Cannot send data. No connection."));
    return;
  }

  if (mDebugLog)
  {
    UE_LOG(
        LogTemp,
        Log,
        TEXT("[CflatDebugAdapter] <<-- Sending Data: %s"),
        *pString);
  }

  if (mSocket->Wait(
          ESocketWaitConditions::WaitForWrite,
          FTimespan::FromSeconds(kSocketWaitTime)))
  {
    const auto ansiString = StringCast<ANSICHAR>(*pString);
    int32 bytesSent = 0;
    mSocket->Send((const uint8*)ansiString.Get(), pString.Len() + 1, bytesSent);
    mSocket->Send(&kMessageEndMarker, 1, bytesSent);
  }
}

uint32 CflatDebugAdapter::Run()
{
  while (true)
  {
    FPlatformProcess::Sleep(kSleepTime);

    CheckListener();

    if (mSocket)
    {
      uint32 pendingDataSize = 0u;
      if (mSocket->HasPendingData(pendingDataSize))
      {
        ReadData();
      }
    }
  }

  return 0;
}

#endif // CFLAT_ENABLED
