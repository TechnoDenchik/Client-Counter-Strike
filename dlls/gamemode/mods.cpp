#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "mods.h"

#include <tuple>
#include <type_traits>

#include "mod_none.h"


IBaseMod *g_pModRunning = nullptr;

template<class T>
IBaseMod *DefaultFactory()
{
	return new T;
}

std::pair<const char *, IBaseMod *(*)()> g_FindList[] = {
	{ "", DefaultFactory<CMod_None> }, // default
	{ "", DefaultFactory<CMod_None> }, // BTE_MOD_CS16
};

void InstallBteMod(const char *name)
{
	for (auto p : g_FindList)
	{
		if (!strcasecmp(name, p.first))
		{
			g_pModRunning = p.second();
			return;
		}
	}
	g_pModRunning = g_FindList[0].second(); // default
	return;
}