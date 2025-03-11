#pragma once

#if defined(CFLAT_ENABLED)
#  include "CoreMinimal.h"

class FSocket;

class CFLAT_API CflatDebugAdapter : public FRunnable
{
public:
  CflatDebugAdapter();
  ~CflatDebugAdapter();

  typedef void (*EventCallback)(const TSharedPtr<FJsonObject>);

  bool Start(int32 pPort = 6663);
  void AddRequestListener(FName pRequestName, EventCallback pCallback);
  TSharedPtr<FJsonObject>
  CreateResponse(const TSharedPtr<FJsonObject> pRequest, bool pSuccess);
  TSharedPtr<FJsonObject> CreateEvent(const FString& pName);

  void SendEvent(const TSharedPtr<FJsonObject> pEvent);
  void SendResponse(const TSharedPtr<FJsonObject> pResponse);

  bool IsConnected();
  bool Disconnect();

protected:
  static const int32 kIncomingBufferSize = 1024;

  ISocketSubsystem* mSocketSubsystem;
  FSocket* mListener;
  FSocket* mSocket;
  double mConnectionLastCheck;
  bool mListening;
  bool mDebugLog;
  FString mListeningAdress;
  int32 mPort;
  uint8 mIncomingBuffer[kIncomingBufferSize];

  FRunnableThread* mThread;
  TMap<FName, EventCallback> mRequestCallbacks;

  bool Listen();

  void CheckListener();

  void ParseMessageData(const uint8* pData, int32 pSize);
  void ReadData();
  void SendString(const FString& pString);

  uint32 Run() override;
};

#endif // CFLAT_ENABLED
