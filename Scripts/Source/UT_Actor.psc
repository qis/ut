; https://ck.uesp.net/wiki/Actor_Script
ScriptName UT_Actor Extends Actor

import UT_Trinity

; https://www.nexusmods.com/skyrimspecialedition/mods/13048
import ActorUtil

ObjectReference Property ContainerArmor Auto
ObjectReference Property ContainerInventory Auto
Actor Property PlayerReference Auto
Race Property RaceInvisible Auto
Race Property RaceSkeleton Auto
Package Property Follow Auto
Int Property Trinity Auto

Event OnInit()
  ; Protect summoner.
  IgnoreFriendlyHits(True)

  ; Move to spawn location.
  Float[] pos = UT_Trinity.GetSpawnLocation(PlayerReference, Trinity)
  MoveTo(PlayerReference, pos[0], pos[1], 0, True)

  ; Move all container items to inventory.
  Form[] forms = ContainerArmor.GetContainerForms()
  Int index = forms.Length
  While index > 0
    index -= 1
    Form item = forms[index]
    ContainerArmor.RemoveItem(item, 1, true, Self)
    EquipItemEx(item, 0, true, false)
  EndWhile
  ContainerInventory.RemoveAllItems(Self, True, True)

  ; Make visible.
  SetRace(RaceSkeleton)
EndEvent

Event OnUpdate()
  ; Update trinity.
  UT_Trinity.Update()
EndEvent

Event OnRaceSwitchComplete()
  If GetRace() == RaceSkeleton
    ; Add package override.
    ActorUtil.AddPackageOverride(Self, Follow, 1, 1)

    ; Add trinity.
    UT_Trinity.Add(Self)

    ; Call OnUpdate every second.
    If Trinity == 3
      RegisterForUpdate(1.0)
    EndIf
    Return
  EndIf

  ; Delete trinity.
  DeleteWhenAble()
EndEvent

Event OnDeath(Actor killer)
  ; Move all inventory items to containers.
  Form[] forms = GetContainerForms()
  Int index = forms.Length
  While index > 0
    index -= 1
    Form item = forms[index]
    If IsEquipped(item) && item.GetType() == 26  ; Armor
      RemoveItem(item, 1, true, ContainerArmor)
    EndIf
  EndWhile
  RemoveAllItems(ContainerInventory, True, True)

  ; Remove trinity.
  UT_Trinity.Remove(Self)

  ; Remove package override.
  ActorUtil.RemovePackageOverride(Self, Follow)

  ; Make invisible.
  If GetRace() != RaceInvisible
    ; Move actor to container.
    Reset(ContainerInventory)

    ; Reset actor race.
    SetRace(RaceInvisible)
  EndIf
EndEvent

Event OnActivate(ObjectReference subject)
  ; Open inventory.
  If subject == PlayerReference
    OpenInventory(True)
  EndIf
EndEvent
