ScriptName UT_Trinity Hidden

Function Add(Actor target) Global Native
Function Remove(Actor target) Global Native
Function Update() Global Native

Int Function GetCount(Actor target) Global
  If target.HasPerk(Game.GetFormFromFile(0x185737, "Requiem.esp") As Perk)
    Return 3
  EndIf
  If target.HasPerk(Game.GetFormFromFile(0x185736, "Requiem.esp") As Perk)
    Return 2
  EndIf
  If target.HasPerk(Game.GetFormFromFile(0x0D5F1C, "Skyrim.esm") As Perk)
    Return 2
  EndIf
  Return 1
EndFunction

Float Function GetAngle(Actor PlayerReference, Int trinity) Global
  If Trinity == 3  ; Warlock
    Return 22
  Else
    Int count = GetCount(PlayerReference)
    If Trinity == 1  ; Guard
      If count == 2
        Return 349
      Else
        Return 0
      EndIf
    ElseIf Trinity == 2  ; Knight
      If count > 2
        Return 338
      Else
        Return 11
      EndIf
    EndIf
  EndIf
  Return 0
EndFunction

Float[] Function GetSpawnLocation(Actor PlayerReference, Int trinity, Float distance = 180.0) Global
  Float[] pos = new Float[2]
  Float angle = GetAngle(PlayerReference, trinity)
  Float angZ = angle + PlayerReference.GetAngleZ()
  pos[0] = distance * Math.sin(angZ)
  pos[1] = distance * Math.cos(angZ)
  Return pos
EndFunction
