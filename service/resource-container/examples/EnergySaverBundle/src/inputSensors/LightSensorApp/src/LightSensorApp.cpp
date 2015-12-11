//******************************************************************
//
// Copyright 2015 Jaemin Jo (Seoul National University) All Rights Reserved.
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include <LightSensorApp.h>

using namespace OC;
using namespace std;

int g_Observation = 0;

OCEntityHandlerResult entityHandler(std::shared_ptr< OCResourceRequest > request);

/*
 * TempResourceFunctions
 */

void LightResource::registerResource()
{
    uint8_t resourceProperty = OC_DISCOVERABLE | OC_OBSERVABLE;

    // This will internally create and register the resource.
    OCStackResult result = OC::OCPlatform::registerResource(m_resourceHandle, m_resourceUri,
                           m_resourceTypes[0], m_resourceInterfaces[0], &entityHandler, resourceProperty);

    if (OC_STACK_OK != result)
    {
        cout << "Resource creation was unsuccessful\n";
    }
}

OCResourceHandle LightResource::getHandle()
{
    return m_resourceHandle;
}

void LightResource::setResourceRepresentation(OCRepresentation &rep)
{
    double tempLight;

    rep.getValue("light", tempLight);

    m_light = tempLight;

    cout << "\t\t\t" << "Received representation: " << endl;
    cout << "\t\t\t\t" << "Light" << m_light << endl;
}

OCRepresentation LightResource::getResourceRepresentation()
{
    m_resourceRep.setValue("light", std::to_string(m_light));

    return m_resourceRep;
}

// Create the instance of the TemphumidResource class
LightResource g_myResource;

void *TestSensorVal(void *param)
{
    (void)param;

    bool bFlag = true;
    int nSleep_time = INTERVAL_FOR_CHECK;
    double nLight;

    std::cout << "[LightSensorAPP] ::" << __func__ << " is called."
              << std::endl;

    nLight = MIN_VAL;

    // This function continuously monitors for the changes
    while (1)
    {
        sleep(nSleep_time);

        if (g_Observation)
        {
            if (bFlag)
            {
                nSleep_time = INTERVAL_FOR_MEASUREMENT;
                nLight = MAX_VAL - nLight;
                g_myResource.m_light = nLight;
            }
            else
            {
                nSleep_time = INTERVAL_FOR_CHECK;
            }

            bFlag = bFlag ^ true;

            cout << "\n light updated to : " << g_myResource.m_light << endl;
            cout << "Notifying observers with resource handle: " << g_myResource.getHandle()
                 << endl;

            OCStackResult result = OCPlatform::notifyAllObservers(g_myResource.getHandle());

            if (OC_STACK_NO_OBSERVERS == result)
            {
                cout << "No More observers, stopping notifications" << endl;
                g_Observation = 0;
            }
        }
        else
        {
            nSleep_time = INTERVAL_FOR_CHECK;
        }

    }
    return NULL;
}

OCEntityHandlerResult entityHandler(std::shared_ptr< OCResourceRequest > request)
{
    cout << "\tIn Server CPP entity handler:\n";

    auto response = std::make_shared<OC::OCResourceResponse>();

    if (request)
    {
        // Get the request type and request flag
        std::string requestType = request->getRequestType();
        int requestFlag = request->getRequestHandlerFlag();

        response->setRequestHandle(request->getRequestHandle());
        response->setResourceHandle(request->getResourceHandle());

        if (requestFlag & RequestHandlerFlag::RequestFlag)
        {
            cout << "\t\trequestFlag : Request\n";

            // If the request type is GET
            if (requestType == "GET")
            {
                cout << "\t\t\trequestType : GET\n";

                // Check for query params (if any)
                // Process query params and do required operations ..

                // Get the representation of this resource at this point and send it as response
                OCRepresentation rep = g_myResource.getResourceRepresentation();

                if (response)
                {
                    // TODO Error Code
                    response->setErrorCode(200);
                    response->setResourceRepresentation(rep, DEFAULT_INTERFACE);
                }
            }
            else if (requestType == "PUT")
            {
                // TODO: PUT request operations
            }
            else if (requestType == "POST")
            {
                // TODO: POST request operations
            }
            else if (requestType == "DELETE")
            {
                // TODO: DELETE request operations
            }
        }

        if (requestFlag & RequestHandlerFlag::ObserverFlag)
        {
            pthread_t threadId;

            cout << "\t\trequestFlag : Observer\n";
            g_Observation = 1;

            static int startedThread = 0;

            if (!startedThread)
            {
                pthread_create(&threadId, NULL, TestSensorVal, (void *) NULL);
                startedThread = 1;
            }
        }
    }
    else
    {
        std::cout << "Request invalid" << std::endl;
    }

    return OCPlatform::sendResponse(response) == OC_STACK_OK ? OC_EH_OK : OC_EH_ERROR;
}

int main()
{
    // Create PlatformConfig object
    PlatformConfig cfg(COAP_SRVTYPE, COAP_MODE, COAP_IP, COAP_PORT, OC::QualityOfService::LowQos);

    try
    {
        OC::OCPlatform::Configure(cfg);

        OC::OCPlatform::startPresence(60);

        g_myResource.registerResource();

        int input = 0;
        cout << "Type any key to terminate" << endl;
        cin >> input;

        OC::OCPlatform::stopPresence();
    }
    catch (std::exception e)
    {
        cout << e.what() << endl;
    }

    return 0;
}
