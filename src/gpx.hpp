#pragma once

#include <stdio.h>
#include <time.h>

/**
 * Wrapper around a GPX file. Will take care of terminating the file properly
 * every time, regardless of whether a track is currently being updated.
 */

class GPXWriter {
public:
	GPXWriter() { _fd = NULL; };
	~GPXWriter() { deinit(); };

	bool init(const char *fname);
	void deinit();

	/**
	 * Start a new GPX track with a given name. If a track with the same name
	 * is already being updated, this method has no effect. On the other hand,
	 * if a track is being updated but has a different name, it will be terminated
	 * 
	 * @param name name of the new track.
	 */
	void startTrack(const char *name);

	/**
	 * Terminate a track. If no track is being recorded, this method has no effect
	 */
	void stopTrack();

	/**
	 * Add a point to the current track. If no track is being recorded, this 
	 * method has no effect.
	 * 
	 * @param time UTC time of the new point
	 * @param lat latitude of the point, in degrees
	 * @param lot longitude of the point, in degrees
	 * @param alt altitude of the point, in meters
	 */
	void addTrackPoint(time_t time, float lat, float lon, float alt);

private:
	void terminateFile();
	void stopTrackInternal();
	FILE *_fd;
	unsigned long _offset;
	bool _trackActive;
	char sondeSerial[64];

	float _lat, _lon, _alt;
};
