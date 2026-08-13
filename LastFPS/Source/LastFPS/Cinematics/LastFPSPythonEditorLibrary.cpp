#include "Cinematics/LastFPSPythonEditorLibrary.h"
#include "MovieSceneSequence.h"
#include "MovieScene.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

bool ULastFPSPythonEditorLibrary::AddSequenceMark(UMovieSceneSequence* Sequence, int32 FrameNumber, const FString& Label)
{
#if WITH_EDITOR
	if (!Sequence)
	{
		return false;
	}

	UMovieScene* MovieScene = Sequence->GetMovieScene();
	if (!MovieScene)
	{
		return false;
	}

	// Check for duplicates
	for (const FMovieSceneMarkedFrame& Mark : MovieScene->GetMarkedFrames())
	{
		if (Mark.Label == Label)
		{
			return true; // Already exists
		}
	}

	MovieScene->Modify();
	FMovieSceneMarkedFrame NewMark;
	NewMark.FrameNumber = FrameNumber;
	NewMark.Label = Label;
	
	MovieScene->AddMarkedFrame(NewMark);
	return true;
#else
	return false;
#endif
}
