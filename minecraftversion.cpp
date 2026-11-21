#include "minecraftversion.h"

MinecraftVersion::MinecraftVersion() {}

MinecraftVersion::MinecraftVersion(QString id,QString type,QString url,QString time,QString releaseTime){
    this->id = id;
    this->type = type;
    this->url = url;
    this->time = time;
    this->releaseTime = releaseTime;
}
