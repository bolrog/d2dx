/*
	This file is part of D2DX.

	Copyright (C) 2021  Bolrog

	D2DX is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	D2DX is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with D2DX.  If not, see <https://www.gnu.org/licenses/>.
*/
#pragma once

namespace d2dx
{
	namespace D2
	{
        enum class UnitType
        {
            Player = 0,
            Monster = 1,
            Object = 2,
            Missile = 3,
            Item = 4,
            VisTile = 5,
            Count = 6,
        };

        struct CellContext109		//size 0x48
        {
            void* pCellFile;		//0x00
            void* pGfxCells;		//0x04
            DWORD direction;		//0x08
            DWORD _1;				//0x0C
            DWORD _2;				//0x10
            DWORD _3;				//0x14
            DWORD _4;				//0x18
            BYTE _5;				//0x1C
            BYTE nComponents;		//0x1D
            WORD _6;				//0x1E
            DWORD dwClass;			//0x20
            DWORD dwUnit;			//0x24
            DWORD dwMode;			//0x28
            DWORD _8;				//0x2C
            DWORD dwUnitToken;		//0x30
            DWORD dwCompToken;		//0x34
            DWORD dwSomeToken;		//0x38
            DWORD _9;				//0x3C
            DWORD dwWClassToken;	//0x40
            char* szName;			//0x44
        };

        static_assert(sizeof(CellContext109) == 0x48, "CellContext109 size");

        struct CellContext112		//size 0x48
        {
            uint32_t _0a;			//0x00
            uint32_t dwClass;		//0x04
            uint32_t dwUnit;		//0x08
            uint32_t dwMode;		//0x0C
            uint32_t _3;			//0x10
            uint32_t dwPlayerType;	//0x14
            BYTE _5;				//0x18
            BYTE _5a;				//0x19
            WORD _6;				//0x1A
            uint32_t _7;			//0x1C
            uint32_t _8;			//0x20
            uint32_t _9;			//0x24
            char* szName;			//0x28
            uint32_t _10;			//0x2C
            uint32_t _11;			//0x30
            void* pCellFile;		//0x34 also pCellInit
            uint32_t _12;			//0x38
            void* pGfxCells;		//0x3C
            uint32_t direction;		//0x40
            uint32_t _14;			//0x44
        };

        static_assert(sizeof(CellContext112) == 0x48, "CellContext112 size");

		struct CellContext113		//size 0x48
		{
			uint32_t nCellNo;		//0x00
			uint32_t _0a;			//0x04
			uint32_t dwClass;		//0x08
			uint32_t dwUnit;		//0x0C
			uint32_t dwMode;		//0x10
			uint32_t _3;			//0x14
			uint32_t dwPlayerType;	//0x18
			BYTE _5;				//0x1C
			BYTE _5a;				//0x1D
			WORD _6;				//0x1E
			uint32_t _7;			//0x20
			uint32_t _8;			//0x24
			uint32_t _9;			//0x28
			char* szName;			//0x2C
			uint32_t _11;			//0x30
			void* pCellFile;		//0x34 also pCellInit
			uint32_t _12;			//0x38
			void* pGfxCells;		//0x3C
			uint32_t direction;		//0x40
			uint32_t _14;			//0x44
		};

		static_assert(sizeof(CellContext113) == 0x48, "CellContext113 size");

        struct CellContextAny {
            union {
                CellContext109 v109;
                CellContext112 v112;
                CellContext113 v113;
            } u;
        };

        struct UnitAny;
        struct Room1;

        struct Path
        {
            DWORD x;    					//0x00
            DWORD y;    					//0x04
            DWORD xUnknown;					//0x08  16 * (wInitX - wInitY) <- Mby AutomapX
            DWORD yUnknown;					//0x0C  8 * (wInitX + wInitY + 1) <- Mby AutoampY
            short xTarget;					//0x10
            short yTarget;					//0x12
            DWORD _2[2];					//0x14
            Room1* pRoom1;					//0x1C
            Room1* pRoomUnk;				//0x20
            DWORD _3[3];					//0x24
            UnitAny* pUnit;					//0x30
            DWORD dwFlags;					//0x34 0x40000 -> PATH_MISSILE_MASK
            DWORD _4;						//0x38
            DWORD dwPathType;				//0x3C
            DWORD dwPrevPathType;			//0x40
            DWORD dwUnitSize;				//0x44
            DWORD _5[2];					//0x48
            DWORD dwCollisionFlag;			//0x50  0x1804 <- bFlying, 0x3401 <- bOpenDoors, 0x3C01 <- Cannot Open Doors, 0x804 <- Ghost, 0x1C09 <- Player
            DWORD _5d;						//0x54
            UnitAny* pTargetUnit;			//0x58
            DWORD dwTargetType;				//0x5C
            DWORD dwTargetId;				//0x60
            BYTE bDirection;				//0x64
        };

        static_assert(sizeof(Path) == 0x68, "Path size");

        struct StaticPath // size 0x20
        {
            Room1* pRoom1;		//0x00
            DWORD xOffset;		//0x04
            DWORD yOffset;		//0x08
            DWORD xPos;			//0x0C
            DWORD yPos;			//0x10
            DWORD _1[2];		//0x14
            DWORD dwFlags;		//0x1C
        };

        static_assert(sizeof(StaticPath) == 0x20, "StaticPath size");
        
        struct Unit109 {
            D2::UnitType dwType;		// 0x00
            DWORD dwClassId;			// 0x04
            DWORD dwUnitId;				// 0x0C
            void* pMemPool;				// 0x08
            DWORD dwMode;				// 0x10
            union {
                void* pPlayerData;
                void* pItemData;
                void* pMonsterData;
                void* pObjectData;
                // TileData *pTileData doesn't appear to exist anymore
            };                          // 0x14
            DWORD dwAct;                // 0x18
            void* pAct;                 // 0x1C
            DWORD dwSeed[2];            // 0x20
            DWORD _2;                   // 0x28
            DWORD _3[3];                // 0x2C
            union {
                Path* path;
                StaticPath* staticPath;
            };                          // 0x38
            DWORD _4[51];               // 0x3C
            struct Unit109* pPrevUnit;  // 0x108
        };

        static_assert(sizeof(Unit109) == 0x10C, "Unit109 size");

        struct Unit110 {
            D2::UnitType dwType;		// 0x00
            DWORD dwClassId;			// 0x04
            void* pMemPool;				// 0x08
            DWORD dwUnitId;				// 0x0C
            DWORD dwMode;				// 0x10
            union {
                void* pPlayerData;
                void* pItemData;
                void* pMonsterData;
                void* pObjectData;
                // TileData *pTileData doesn't appear to exist anymore
            };                          // 0x14
            DWORD dwAct;                // 0x18
            void* pAct;                 // 0x1C
            DWORD dwSeed[2];            // 0x20
            DWORD _2;                   // 0x28
            union {
                Path* path;
                StaticPath* staticPath;
            };                          // 0x2C
            DWORD _3[3];                // 0x30
            DWORD _4[51];               // 0x3C
            struct Unit110* pPrevUnit;  // 0x108
        };

        static_assert(sizeof(Unit110) == 0x10C, "Unit110 size");

        struct Unit112 {
            UnitType dwType;            // 0x00
            DWORD dwTxtFileNo;          // 0x04
            DWORD _1;                   // 0x08
            DWORD dwUnitId;             // 0x0C
            DWORD dwMode;               // 0x10
            union {
                void* pPlayerData;
                void* pItemData;
                void* pMonsterData;
                void* pObjectData;
                // TileData *pTileData doesn't appear to exist anymore
            };                          // 0x14
            DWORD dwAct;                // 0x18
            void* pAct;                 // 0x1C
            DWORD dwSeed[2];            // 0x20
            DWORD _2;                   // 0x28
            union {
                Path* path;
                StaticPath* staticPath;
            };                          // 0x2C
            DWORD _3[3];                // 0x30
            DWORD unk[2];
            DWORD dwGfxFrame;           // 0x44
            DWORD dwFrameRemain;        // 0x48
            WORD wFrameRate;            // 0x4C
            WORD _4;                    // 0x4E
            BYTE* pGfxUnk;              // 0x50
            DWORD* pGfxInfo;            // 0x54
            DWORD _5;                   // 0x58
            void* pStats;               // 0x5C
            void* pInventory;           // 0x60
            void* ptLight;              // 0x64
            DWORD dwStartLightRadius;   // 0x68
            WORD nPl2ShiftIdx;          // 0x6C
            WORD nUpdateType;           // 0x6E
            UnitAny* pUpdateUnit;       // 0x70 - Used when updating unit.
            DWORD* pQuestRecord;        // 0x74
            DWORD bSparklyChest;        // 0x78 bool
            DWORD* pTimerArgs;          // 0x7C
            DWORD dwSoundSync;          // 0x80
            DWORD _6[2];                // 0x84
            WORD wX;                    // 0x8C
            WORD wY;                    // 0x8E
            DWORD _7;                   // 0x90
            DWORD dwOwnerType;          // 0x94
            DWORD dwOwnerId;            // 0x98
            DWORD _8[2];                // 0x9C
            void* pOMsg;                // 0xA4
            void* pInfo;                // 0xA8
            DWORD _9[6];                // 0xAC
            DWORD dwFlags;              // 0xC4
            DWORD dwFlags2;             // 0xC8
            DWORD _10[5];               // 0xCC
            UnitAny* pChangedNext;      // 0xE0
            UnitAny* pListNext;         // 0xE4 -> 0xD8
            UnitAny* pRoomNext;         // 0xE8                
        };

        static_assert(sizeof(Unit112) == 0xEC, "Unit112 size");

        struct UnitAny 
        {
            union
            {
                struct Unit109 v109;
                struct Unit110 v110;
                struct Unit112 v112;
            } u;
        };

        #pragma pack(push, 1)
        struct Room1 {
            Room1** pRoomsNear; 	//0x00 pptVisibleRooms
            DWORD _1;				//0x04
            void* _1s;				//0x08
            DWORD _1b;				//0x0C
            void* pRoom2;			//0x10
            DWORD _2[2];			//0x14
            UnitAny** pUnitChanged; //0x1C
            void* Coll;			//0x20
            DWORD dwRoomsNear;		//0x24 dwVisibleRooms
            DWORD nPlayerUnits;		//0x28
            void* pAct;				//0x2C
            DWORD _4;				//0x30
            DWORD nUnknown;			//0x34
            DWORD _5[4];			//0x38
            void** pClients;  //0x48
            uint8_t hCoords[0x20];	//0x4C
            int64_t hSeed;			//0x6C
            UnitAny* pUnitFirst;	//0x74
            DWORD nNumClients;		//0x78
            Room1* pRoomNext;		//0x7C
        };
        #pragma pack(pop)

        static_assert(sizeof(Room1) == 0x80, "Room1 size");

        struct Vertex
        {
            float x, y;
            uint32_t color;
            uint32_t padding;
            float s, t;
            uint32_t padding2;
        };
    }
}
