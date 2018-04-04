#include "controlbase.h"

#include <sys/mman.h>

using namespace std::chrono;
ControlBase::ControlBase(/*int argc, char* argv[]*/)
{
    mDataRepository = DataRepository::instance();
}

ControlBase::~ControlBase()
{

}

void ControlBase::registerLogVariable(double *pVariable,
                                      const std::string& pName,
                                      unsigned int pRow,
                                      unsigned int pCol,
                                      const std::string& pDesc)
{
    mDataRepository->insertLogVariable(
                new LogVariable( pVariable, pName, pRow, pCol, pDesc )
                );
}

void ControlBase::registerLogVariable(double& pVariable,
                                      const std::string& pName,
                                      const std::string& pDesc)
{
    registerLogVariable(&pVariable, pName, 1, 1, pDesc);
}

void ControlBase::registerControlVariable(double *pVariable,
                                          const std::string& pName,
                                          unsigned int pRow,
                                          unsigned int pCol,
                                          const std::string& pDesc)
{
    mDataRepository->insertControlVariable(
                new ControlVariable( pVariable, pName, pRow, pCol, pDesc )
                );
}

void ControlBase::registerControlVariable(double& pVariable,
                                          const std::string& pName,
                                          const std::string& pDesc)
{
    registerControlVariable( &pVariable, pName, 1, 1, pDesc );
}


void ControlBase::run(int argc, char *argv[])
{
    if ( argc != 2 )
    {
        std::cout << "Invalid argument" << std::endl;
        return;
    }

    mlockall ( MCL_CURRENT | MCL_FUTURE );
	try
	{
        mDataRepository->setProjectName( argv[1] );

        mLifeCycleTask = new LifeCycleTask( this,
                                   mDataRepository->projectName() +
                                   "LifeCycleTask");
        mLifeCycleTask->join(); // wait for execution to finish
		delete mLifeCycleTask;
	}
    catch (ZnmException e)
	{
        std::cout << "Life Task Error occurred: " << std::string(e.what())
                  << std::endl;
	}
}


//============================================================================//
//		INITIALIZE OPERATIONS												  //
//============================================================================//
void ControlBase::initializeControlBase()
{
    try
    {
        int error = initialize();	// User Function
        if( error )
        {
            std::cerr << "The initialize() function returned non zero: "
                      <<
                         error << std::endl;
        }
    }
    catch( std::exception& e )
    {
        std::cerr << "An exception occured in the initialize() function: "
                  << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "An unknown exception occured in the initialize()"
                     " function." << std::endl;
    }

    mDataRepository->writeVariablesToFile();
    mDataRepository->bindMessageQueues();
    mDataRepository->sendStateRequest( R_INIT );
    // Send message to GUI to read variables
    mDataRepository->bindMainControlHeap();

    // Control Variable degerleri heap'e kopyalanir.
    for (size_t i = 0; i < mDataRepository->controlVariables().size(); ++i)
    {
        mDataRepository->controlVariables()[i]->copyToHeap();
    }

    mDataRepository->sendStateRequest( R_INIT );
    // Send message to GUI to read values
    mState = STOPPED;
}

//============================================================================//
//		START OPERATIONS													  //
//============================================================================//
void ControlBase::startControlBase()
{
	if( mState != RUNNING )
    {
        mDataRepository->bindLogVariablesHeap();
        syncMainHeap();

        int error = 0;

        try
        {
            error = start();	// User Function

            // start() hata ile donerse program baslatilmaz.
            if ( error )
            {
                std::cerr << "The start() function returned non zero: " <<
                             error << std::endl;
            }
        }
        catch( std::exception& e )
        {
            error = -1;
            std::cerr << "An exception occured in the start() function: " <<
                         e.what() << std::endl;
        }
        catch (...)
        {
            error = -1;
            std::cerr << "An unknown exception occured in the start() "
                         "function." << std::endl;
        }

        // start() hata ile donerse program baslatilmaz.
        if ( error )
        {
            mState = STOPPED;
            DataRepository::instance()->sendStateRequest( R_STOP );
            mDataRepository->unbindLogVariableHeap();
        }
        else
        {
            mState = RUNNING;

            mLoopTask = new LoopTask(
                        this,mDataRepository->projectName() + "LoopTask"
                        );
        }
    }
}

void ControlBase::pauseControlBase()
{
    mState = PAUSED;
}

void ControlBase::resumeControlBase()
{
	mState = RUNNING;
}

//============================================================================//
//		LOOP OPERATIONS														  //
//============================================================================//
void ControlBase::syncMainHeap()
{
    for (size_t i = 0; i < mDataRepository->controlVariables().size(); ++i)
    {
        mDataRepository->controlVariables()[i]->copyFromHeap();
    }

    mDuration = mDataRepository->duration();

    mDataRepository->setElapsedTimeSecond( mLifeCycleTask->elapsedTime() );
    mDataRepository->setOverruns( mLoopTask->overruns() );
}


void ControlBase::logVariables( steady_clock::duration pSimTime )
{
    mDataRepository->sampleLogVariable( pSimTime );
}


//============================================================================//
//		STOP OPERATIONS														  //
//============================================================================//
void ControlBase::stopControlBase()
{
    if( mState != STOPPED )
    {
        mState = STOPPED;
        mLoopTask->join();
        delete mLoopTask;

        mDataRepository->unbindLogVariableHeap();

        try
        {
            int error = stop();			// User Function
            if( error )
            {
                std::cerr << "The stop() function returned non zero: "
                          << error << std::endl;
            }
        }
        catch( std::exception& e )
        {
            std::cerr << "An exception occured in the stop() function: "
                      << e.what() << std::endl;
        }
        catch (...)
        {
            std::cerr << "An unknown exception occured in the stop() function."
                      << std::endl;
        }
    }
}

//============================================================================//
//		TERMINATE OPERATIONS												  //
//============================================================================//
void ControlBase::terminateControlBase()
{
    mDataRepository->unbindMessageQueues();
    mDataRepository->unbindMainControlHeap();

    mState = TERMINATED;
	terminate();	// User Function
}

