; https://ck.uesp.net/wiki/ActiveMagicEffect_Script
ScriptName UT_Effect extends ActiveMagicEffect

import UT_Trinity

; https://github.com/powerof3/PapyrusExtenderSSE/wiki
Import PO3_SKSEFunctions

ActorBase Property ActorGuard Auto
ActorBase Property ActorKnight Auto
ActorBase Property ActorWarlock Auto
Actor Property PlayerReference Auto
Spell Property SummonGuard Auto
Spell Property SummonKnight Auto
Spell Property SummonWarlock Auto
Form Property Visual Auto

ObjectReference[] Visuals
Int VisualsLength = 0

Function CreateVisual(Int trinity)
  Visuals[VisualsLength] = PlayerReference.PlaceAtMe(Visual, 1, False, True)
  Float[] pos = UT_Trinity.GetSpawnLocation(PlayerReference, trinity)
  Visuals[VisualsLength].MoveTo(PlayerReference, pos[0], pos[1], 0, True)
  Visuals[VisualsLength].Enable(False)
  VisualsLength += 1
EndFunction

Function Summon(Actor target, Int trinity)
  Visuals[VisualsLength] = target.PlaceAtMe(Visual)
  VisualsLength += 1
  Float[] pos = GetSpawnLocation(PlayerReference, trinity)
  target.MoveTo(PlayerReference, pos[0], pos[1], 0, True)
  CreateVisual(trinity)
EndFunction

Event OnEffectStart(Actor target, Actor caster)
  ; Get summoned trinity actors.
  Actor guard = None
  Actor knight = None
  Actor warlock = None
  Actor[] commandedActors = PO3_SKSEFunctions.GetCommandedActors(caster)
  Int index = commandedActors.Length
  While index > 0
    index -= 1
    Actor commandedActor = commandedActors[index]
    If commandedActor != None
      ActorBase base = commandedActor.GetBaseObject() As ActorBase
      If base == ActorGuard
        guard = commandedActor
      ElseIf base == ActorKnight
        knight = commandedActor
      ElseIf base == ActorWarlock
        warlock = commandedActor
      EndIf
    EndIf
  EndWhile

  ; Create visuals array.
  Visuals = new ObjectReference[6]
  VisualsLength = 0

  ; Summon guard.
  If guard == None
    CreateVisual(1)
    SummonGuard.RemoteCast(PlayerReference, None, None)
  Else
    Summon(guard, 1)
  EndIf

  ; Summon knight.
  If knight == None
    If UT_Trinity.GetCount(PlayerReference) > 1
      CreateVisual(2)
      SummonKnight.RemoteCast(PlayerReference, None, None)
    EndIf
  Else
    Summon(knight, 2)
  EndIf

  ; Summon warlock.
  If warlock == None
    If UT_Trinity.GetCount(PlayerReference) > 2
      CreateVisual(3)
      SummonWarlock.RemoteCast(PlayerReference, None, None)
    EndIf
  Else
    Summon(warlock, 3)
  EndIf

  ; Dispel effect in 3.5 seconds.
  RegisterForSingleUpdate(3.5)
EndEvent

Event OnUpdate()
  Dispel()
EndEvent

Event OnEffectFinish(Actor target, Actor caster)
  ; Delete visuals.
  While VisualsLength > 0
    VisualsLength -= 1
    If Visuals[VisualsLength] != None
      Visuals[VisualsLength].Delete()
      Visuals[VisualsLength] = None
    EndIf
  EndWhile
EndEvent
