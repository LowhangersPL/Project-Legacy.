#include "scriptPCH.h"
#include "kara.h"

enum
{
    SPELL_SHADOW_BOLT = 11661,
    SPELL_GREATER_HEAL = 10965,
    SPELL_CURSE_OF_THE_DARKMASTER = 18702,
    SPELL_MANA_BURN = 10876,
    SPELL_SHADOW_BOLT_VOLLEY = 27646,
    SPELL_FROSTBOLT = 10181,
    SPELL_FEAR = 6215,
    SPELL_CURSE_OF_RECKLESSNESS = 704,
    SPELL_CURSE_OF_TONGUES = 11719,
    SPELL_CURSE_OF_WEAKNESS = 11708,

    SPELL_CLEAVE = 26350,
    SPELL_DEATH_COIL = 17926,

    SPELL_SHIELD_SLAM = 23925,
    SPELL_DISARM = 676,
};

struct npc_captain_aldrenAI : public ScriptedAI
{
    npc_captain_aldrenAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiShadowBoltTimer;
    uint32 m_uiCurseTonguesTimer;
    uint32 m_uiCurseWeaknessTimer;

    void Reset() override
    {
        m_uiShadowBoltTimer = urand(2000, 4000);
        m_uiCurseTonguesTimer = urand(5000, 8000);
        m_uiCurseWeaknessTimer = urand(8000, 11000);

        m_creature->SetCasterChaseDistance(25.0f);
    }

    void UpdateAI(uint32 const uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        Unit* pVictim = m_creature->GetVictim();

        if (m_uiCurseTonguesTimer <= uiDiff)
        {
            Unit* pTarget = nullptr;

            for (uint8 i = 0; i < 10; ++i)
            {
                Unit* pTest = m_creature->SelectAttackingTarget(ATTACKING_TARGET_TOPAGGRO, i);

                if (pTest && pTest->GetPowerType() == POWER_MANA)
                {
                    pTarget = pTest;
                    break;
                }
            }

            if (pTarget && DoCastSpellIfCan(pTarget, SPELL_CURSE_OF_TONGUES) == CAST_OK)
            {
                m_uiCurseTonguesTimer = urand(18000, 24000);
                return;
            }

            m_uiCurseTonguesTimer = 2000;
        }
        else
            m_uiCurseTonguesTimer -= uiDiff;

        if (m_uiCurseWeaknessTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(pVictim, SPELL_CURSE_OF_WEAKNESS) == CAST_OK)
            {
                m_uiCurseWeaknessTimer = urand(20000, 26000);
                return;
            }
        }
        else
            m_uiCurseWeaknessTimer -= uiDiff;

        if (m_uiShadowBoltTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(pVictim, SPELL_SHADOW_BOLT) == CAST_OK)
            {
                m_uiShadowBoltTimer = urand(3000, 5000);
                return;
            }
        }
        else
            m_uiShadowBoltTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};


struct npc_captain_bartholAI : public ScriptedAI
{
    npc_captain_bartholAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiCleaveTimer;
    uint32 m_uiDeathCoilTimer;

    void Reset() override
    {
        m_uiCleaveTimer = urand(3000, 5000);
        m_uiDeathCoilTimer = urand(7000, 9000);
    }

    void UpdateAI(uint32 const uiDiff) override
    {
        if (!UpdateVictim())
            return;

        if (m_uiDeathCoilTimer <= uiDiff)
        {
            Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0);

            if (pTarget && DoCastSpellIfCan(pTarget, SPELL_DEATH_COIL) == CAST_OK)
            {
                m_uiDeathCoilTimer = urand(12000, 15000);
                return;
            }

            m_uiDeathCoilTimer = 2000;
        }
        else
            m_uiDeathCoilTimer -= uiDiff;

        if (m_uiCleaveTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_CLEAVE) == CAST_OK)
            {
                m_uiCleaveTimer = urand(7000, 9000);
                return;
            }
        }
        else
            m_uiCleaveTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

struct npc_captain_renwaldAI : public ScriptedAI
{
    npc_captain_renwaldAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiShieldSlamTimer;
    uint32 m_uiDisarmTimer;

    void Reset() override
    {
        m_uiShieldSlamTimer = urand(3000, 5000);
        m_uiDisarmTimer = urand(9000, 12000);
    }

    void UpdateAI(uint32 const uiDiff) override
    {
        if (!UpdateVictim())
            return;

        Unit* pVictim = m_creature->GetVictim();

        if (m_uiDisarmTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(pVictim, SPELL_DISARM) == CAST_OK)
            {
                m_uiDisarmTimer = urand(20000, 25000);
                return;
            }
        }
        else
            m_uiDisarmTimer -= uiDiff;

        if (m_uiShieldSlamTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(pVictim, SPELL_SHIELD_SLAM) == CAST_OK)
            {
                m_uiShieldSlamTimer = urand(8000, 10000);
                return;
            }
        }
        else
            m_uiShieldSlamTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

/*
 * Crypt Caretaker
 */

struct npc_crypt_caretakerAI : public ScriptedAI
{
    npc_crypt_caretakerAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiShadowBoltTimer;
    uint32 m_uiGreaterHealTimer;
    uint32 m_uiCurseTimer;

    void Reset() override
    {
        m_uiShadowBoltTimer = urand(2000, 4000);
        m_uiGreaterHealTimer = urand(6000, 9000);
        m_uiCurseTimer = urand(5000, 8000);

        m_creature->SetCasterChaseDistance(25.0f);
    }

    void UpdateAI(uint32 const uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        Unit* pVictim = m_creature->GetVictim();

        // Heal another friendly creature below 60% health first.
        if (m_uiGreaterHealTimer <= uiDiff)
        {
            Unit* pHealTarget = m_creature->FindLowestHpFriendlyUnit(
                30.0f,
                40,
                true,
                m_creature
            );

            if (pHealTarget)
            {
                if (DoCastSpellIfCan(pHealTarget, SPELL_GREATER_HEAL) == CAST_OK)
                {
                    m_uiGreaterHealTimer = urand(10000, 14000);
                    return;
                }
            }

            // If no ally needs healing, heal itself below 60%.
            if (m_creature->GetHealthPercent() < 60.0f)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_GREATER_HEAL) == CAST_OK)
                {
                    m_uiGreaterHealTimer = urand(10000, 14000);
                    return;
                }
            }

            m_uiGreaterHealTimer = 2000;
        }
        else
            m_uiGreaterHealTimer -= uiDiff;

        // Curse the current target.
        if (m_uiCurseTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(pVictim, SPELL_CURSE_OF_THE_DARKMASTER) == CAST_OK)
            {
                m_uiCurseTimer = urand(18000, 24000);
                return;
            }
        }
        else
            m_uiCurseTimer -= uiDiff;

        // Continue casting Shadow Bolt even in melee range.
        if (m_uiShadowBoltTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(pVictim, SPELL_SHADOW_BOLT) == CAST_OK)
            {
                m_uiShadowBoltTimer = urand(4000, 6000);
                return;
            }
        }
        else
            m_uiShadowBoltTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_crypt_caretaker(Creature* pCreature)
{
    return new npc_crypt_caretakerAI(pCreature);
}

/*
 * Crypt Acolyte
 */

struct npc_crypt_acolyteAI : public ScriptedAI
{
    npc_crypt_acolyteAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiShadowBoltTimer;
    uint32 m_uiManaBurnTimer;
    uint32 m_uiCurseTimer;

    void Reset() override
    {
        m_uiShadowBoltTimer = urand(2000, 4000);
        m_uiManaBurnTimer = urand(6000, 9000);
        m_uiCurseTimer = urand(5000, 8000);

        m_creature->SetCasterChaseDistance(25.0f);
    }

    void UpdateAI(uint32 const uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        Unit* pVictim = m_creature->GetVictim();

        // Prefer the current victim when it uses mana, otherwise find another mana user.
        if (m_uiManaBurnTimer <= uiDiff)
        {
            Unit* pManaTarget = nullptr;

            if (pVictim->GetPowerType() == POWER_MANA)
                pManaTarget = pVictim;
            else
            {
                // Search the threat list for another unit that uses mana.
                // Using the two-argument overload avoids the ambiguous-call
                // compiler error on this MaNGOS branch.
                for (uint8 i = 0; i < 10; ++i)
                {
                    Unit* pTarget = m_creature->SelectAttackingTarget(
                        ATTACKING_TARGET_TOPAGGRO,
                        i
                    );

                    if (pTarget && pTarget->GetPowerType() == POWER_MANA)
                    {
                        pManaTarget = pTarget;
                        break;
                    }
                }
            }

            if (pManaTarget)
            {
                if (DoCastSpellIfCan(pManaTarget, SPELL_MANA_BURN) == CAST_OK)
                {
                    m_uiManaBurnTimer = urand(10000, 14000);
                    return;
                }

                // Fallback for creatures whose template power or spell checks reject the cast.
                m_creature->CastSpell(pManaTarget, SPELL_MANA_BURN, true);
                m_uiManaBurnTimer = urand(10000, 14000);
                return;
            }

            // No mana user is currently on the threat list.
            m_uiManaBurnTimer = 2000;
        }
        else
            m_uiManaBurnTimer -= uiDiff;

        // Curse the current target.
        if (m_uiCurseTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(pVictim, SPELL_CURSE_OF_THE_DARKMASTER) == CAST_OK)
            {
                m_uiCurseTimer = urand(18000, 24000);
                return;
            }
        }
        else
            m_uiCurseTimer -= uiDiff;

        // Continue casting Shadow Bolt even in melee range.
        if (m_uiShadowBoltTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(pVictim, SPELL_SHADOW_BOLT) == CAST_OK)
            {
                m_uiShadowBoltTimer = urand(4000, 6000);
                return;
            }
        }
        else
            m_uiShadowBoltTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_crypt_acolyte(Creature* pCreature)
{
    return new npc_crypt_acolyteAI(pCreature);
}

/*
 * Crypt Magus
 */

struct npc_crypt_magusAI : public ScriptedAI
{
    npc_crypt_magusAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiShadowBoltTimer;
    uint32 m_uiShadowBoltVolleyTimer;
    uint32 m_uiCurseTimer;

    void Reset() override
    {
        m_uiShadowBoltTimer = urand(2000, 4000);
        m_uiShadowBoltVolleyTimer = urand(8000, 12000);
        m_uiCurseTimer = urand(5000, 8000);

        m_creature->SetCasterChaseDistance(25.0f);
    }

    void UpdateAI(uint32 const uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        Unit* pVictim = m_creature->GetVictim();

        // Highest priority: Shadow Bolt Volley.
        if (m_uiShadowBoltVolleyTimer <= uiDiff)
        {
            // Use the hostile target rather than the caster as the explicit spell target.
            if (DoCastSpellIfCan(pVictim, SPELL_SHADOW_BOLT_VOLLEY) == CAST_OK)
            {
                m_uiShadowBoltVolleyTimer = urand(12000, 16000);
                return;
            }

            // Fallback in case the spell's normal target or power checks reject the cast.
            m_creature->CastSpell(pVictim, SPELL_SHADOW_BOLT_VOLLEY, true);
            m_uiShadowBoltVolleyTimer = urand(12000, 16000);
            return;
        }
        else
            m_uiShadowBoltVolleyTimer -= uiDiff;

        // Curse the current target.
        if (m_uiCurseTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(pVictim, SPELL_CURSE_OF_THE_DARKMASTER) == CAST_OK)
            {
                m_uiCurseTimer = urand(18000, 24000);
                return;
            }
        }
        else
            m_uiCurseTimer -= uiDiff;

        // Shadow Bolt filler, including when forced into melee.
        if (m_uiShadowBoltTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(pVictim, SPELL_SHADOW_BOLT) == CAST_OK)
            {
                m_uiShadowBoltTimer = urand(3000, 5000);
                return;
            }
        }
        else
            m_uiShadowBoltTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

/*
 * Condemned Shade
 */

struct npc_condemned_shadeAI : public ScriptedAI
{
    npc_condemned_shadeAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiShadowBoltTimer;
    uint32 m_uiCurseTimer;
    uint32 m_uiFearTimer;

    void Reset() override
    {
        m_uiShadowBoltTimer = urand(2000, 4000);
        m_uiCurseTimer = urand(6000, 9000);
        m_uiFearTimer = urand(10000, 15000);

        m_creature->SetCasterChaseDistance(25.0f);
    }

    void UpdateAI(uint32 const uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        /*
         * Highest priority: fear a random player.
         *
         * ATTACKING_TARGET_RANDOM selects from the Shade's threat list.
         * Passing 0 means the current tank can also be selected.
         */
        if (m_uiFearTimer <= uiDiff)
        {
            Unit* pFearTarget = m_creature->SelectAttackingTarget(
                ATTACKING_TARGET_RANDOM,
                0
            );

            if (pFearTarget)
            {
                if (DoCastSpellIfCan(pFearTarget, SPELL_FEAR) == CAST_OK)
                {
                    m_uiFearTimer = urand(20000, 30000);
                    return;
                }
            }

            // No valid target or the cast failed, so retry shortly.
            m_uiFearTimer = 2000;
        }
        else
            m_uiFearTimer -= uiDiff;

        // Apply Curse of Recklessness to the current target.
        if (m_uiCurseTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(
                m_creature->GetVictim(),
                SPELL_CURSE_OF_RECKLESSNESS
            ) == CAST_OK)
            {
                m_uiCurseTimer = urand(15000, 20000);
                return;
            }
        }
        else
            m_uiCurseTimer -= uiDiff;

        // Use the same Shadow Bolt as the other Karazhan casters.
        if (m_uiShadowBoltTimer <= uiDiff)
        {
            if (DoCastSpellIfCan(
                m_creature->GetVictim(),
                SPELL_SHADOW_BOLT
            ) == CAST_OK)
            {
                m_uiShadowBoltTimer = urand(3000, 5000);
                return;
            }
        }
        else
            m_uiShadowBoltTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_condemned_shade(Creature* pCreature)
{
    return new npc_condemned_shadeAI(pCreature);
}

/*
 * Drowned Spirit
 */

struct npc_drowned_spiritAI : public ScriptedAI
{
    npc_drowned_spiritAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 m_uiFrostboltTimer;

    void Reset() override
    {
        m_uiFrostboltTimer = urand(1000, 2500);
        m_creature->SetCasterChaseDistance(25.0f);
    }

    void UpdateAI(uint32 const uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        if (m_uiFrostboltTimer <= uiDiff)
        {
            Unit* pVictim = m_creature->GetVictim();

            if (DoCastSpellIfCan(pVictim, SPELL_FROSTBOLT) == CAST_OK)
            {
                m_uiFrostboltTimer = urand(3000, 4500);
                return;
            }

            // Fallback for Drowned Spirit templates that cannot pay the spell's mana cost.
            m_creature->CastSpell(pVictim, SPELL_FROSTBOLT, true);
            m_uiFrostboltTimer = urand(3000, 4500);
            return;
        }
        else
            m_uiFrostboltTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_drowned_spirit(Creature* pCreature)
{
    return new npc_drowned_spiritAI(pCreature);
}

CreatureAI* GetAI_npc_crypt_magus(Creature* pCreature)
{
    return new npc_crypt_magusAI(pCreature);
}

CreatureAI* GetAI_npc_captain_aldren(Creature* pCreature)
{
    return new npc_captain_aldrenAI(pCreature);
}

CreatureAI* GetAI_npc_captain_barthol(Creature* pCreature)
{
    return new npc_captain_bartholAI(pCreature);
}

CreatureAI* GetAI_npc_captain_renwald(Creature* pCreature)
{
    return new npc_captain_renwaldAI(pCreature);
}

/*
 * Script registration
 */

void AddSC_kara()
{
    Script* pNewScript;

    pNewScript = new Script;
    pNewScript->Name = "npc_crypt_caretaker";
    pNewScript->GetAI = &GetAI_npc_crypt_caretaker;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "npc_crypt_acolyte";
    pNewScript->GetAI = &GetAI_npc_crypt_acolyte;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "npc_crypt_magus";
    pNewScript->GetAI = &GetAI_npc_crypt_magus;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "npc_drowned_spirit";
    pNewScript->GetAI = &GetAI_npc_drowned_spirit;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "npc_condemned_shade";
    pNewScript->GetAI = &GetAI_npc_condemned_shade;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "npc_captain_aldren";
    pNewScript->GetAI = &GetAI_npc_captain_aldren;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "npc_captain_barthol";
    pNewScript->GetAI = &GetAI_npc_captain_barthol;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "npc_captain_renwald";
    pNewScript->GetAI = &GetAI_npc_captain_renwald;
    pNewScript->RegisterSelf();
}