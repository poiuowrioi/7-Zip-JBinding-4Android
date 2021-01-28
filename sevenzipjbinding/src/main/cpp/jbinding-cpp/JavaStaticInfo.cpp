#include "SevenZipJBinding.h"
#include "JavaStaticInfo.h"
#include "JavaStatInfos/JavaStandardLibrary.h"
#include <android/log.h>

namespace jni {

jobject sevenZipClassLoader;


jclass findSevenZipClass(JNIEnv * env, const char * className) {
	char canonicalName[256];
	FATALIF3(sizeof(canonicalName) <= strlen(className), "Local buffer is to small. 7-Zip classname len: %i. Buffer len: %i (%s)",
			 strlen(className), sizeof(canonicalName), className);
	if (sizeof(canonicalName) <= strlen(className)) {
		__android_log_print(ANDROID_LOG_ERROR, "TRACE", "Local buffer is to small. 7-Zip classname len: %i. Buffer len: %i (%s)", strlen(className), sizeof(canonicalName), className);
	}

	strncpy(canonicalName, className, sizeof(canonicalName));
	char * pos = strchr(canonicalName, '/');
	while (pos) {
		*pos = '.';
		pos = strchr(pos, '/');
	}
	TRACE("Canonical class name: '" << canonicalName << "'")
	__android_log_print(ANDROID_LOG_INFO, "TRACE", "Canonical class name: '%s'", canonicalName);

	jstring name = env->NewStringUTF(canonicalName);
	FATALIF(!name, "OutOfMemoryError (findSevenZipClass)")
	if (!name) {
		__android_log_print(ANDROID_LOG_ERROR, "TRACE", "OutOfMemoryError (findSevenZipClass)");
	}

	jclass clazz = (jclass)ClassLoader::loadClass(env, sevenZipClassLoader, name);
	TRACE("Clazz: " << clazz)
	__android_log_print(ANDROID_LOG_INFO, "TRACE", "Clazz: '%p'", clazz);

	prepareExceptionCheck(env);
	jobject exception = env->ExceptionOccurred();
	env->ExceptionClear();
	TRACE("Exception: " << exception)

    __android_log_print(ANDROID_LOG_INFO, "TRACE", "Exception: '%p'", exception);
    if (exception) {
		Throwable::printStackTrace(env, exception);
		prepareExceptionCheck(env);
		env->ExceptionClear();
		FATALIF2(TRUE, "Can't load class with 7-Zip classloader: '%s' (%s)", canonicalName, className)
		__android_log_print(ANDROID_LOG_ERROR, "TRACE", "Can't load class with 7-Zip classloader: '%s' (%s)", canonicalName, className);
	}

	return clazz;
}

jclass findClass(JNIEnv * env, const char * className) {
	if (className == strstr(className, SEVEN_ZIP_PACKAGE)) {
		TRACE("env->FindClass() for 7-Zip class " << className)
		__android_log_print(ANDROID_LOG_INFO, "TRACE", "env->FindClass() for 7-Zip class %s", className);
		return findSevenZipClass(env, className);
	}

	TRACE("env->FindClass() for " << className)
	__android_log_print(ANDROID_LOG_INFO, "TRACE", "env->FindClass() for  %s", className);

	jclass clazz = env->FindClass(className);
	FATALIF1(!clazz, "Can't load class: '%s'", className)
	if (!clazz) {
		__android_log_print(ANDROID_LOG_INFO, "TRACE", "Can't load class: '%s'", className);
	}

	return clazz;
}

void JMethod::initMethodIDIfNecessary(JNIEnv * env, jclass jclazz) {
	if (isInitialized) {
		return;
	}

	_initCriticalSection.Enter();

	if (isInitialized) {
		return;
	}

	initMethodID(env, jclazz);
	isInitialized = true;

	_initCriticalSection.Leave();

}

void JMethod::initMethodID(JNIEnv * env, jclass jclazz) {
	TRACE("Getting method id for " << *this);
	if (_isStatic) {
		_jmethodID = env->GetStaticMethodID(jclazz, _name, _signature);
	} else {
		_jmethodID = env->GetMethodID(jclazz, _name, _signature);
	}

	if (env->ExceptionOccurred()) {
		jthrowable exception = env->ExceptionOccurred();
		env->ExceptionClear();
		if (jni::NoSuchMethodError::_isInstance(env, exception)) {
			return;
		}
		if (jni::OutOfMemoryError::_isInstance(env, exception)) {
			FATAL("Out of memory during method lookup: '%s', '%s'", _name, _signature); // TODO Change fatal => exception (+test)
		}
		if (jni::ExceptionInInitializerError::_isInstance(env, exception)) {
			FATAL("Exception in initializer during method lookup: '%s', '%s'", _name, _signature); // TODO Change fatal => exception (+test)
		}
		FATAL("Unknown exception: '%s', '%s'", _name, _signature);
	}
}

}
