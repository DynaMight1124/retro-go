static const u8 Debug0[] = {
    _("ＳＴＡＧＥ　ＳＥＬＥＣＴ\n"
      "　つづける？\n"
      "　１　マウンテン\n"
      "　２　ファイアーバブル\n"
      "　３　スノースライダー\n"
      "　４　ウォーターランド\n"
      "　　　クッパ１ごう\n"
      "　もどる")
};
static const u8 Debug1[] = {
    _("ＰＡＵＳＥ　　　　\n"
      "　つづける？\n"
      "　やめる　？")
};
static const struct DialogEntry debug_text_entry_0 = {
    1, 8, 30, 200, Debug0
};
static const struct DialogEntry debug_text_entry_1 = {
    1, 3, 100, 150, Debug1
};
const struct DialogEntry *const seg2_debug_text_table[] = {
    &debug_text_entry_0, &debug_text_entry_1, NULL,
};
static const u8 dialog_text_DIALOG_000[] = { _("Wow! You're smack in the\nmiddle of the battlefield.\nYou'll find the Power\nStars that Bowser stole\ninside the painting\nworlds.\nFirst, talk to the\nBob-omb Buddy. (Press [B]\nto talk.) He'll certainly\nhelp you out, and so will\nhis " "comrades" " in other\nareas.\nTo read signs, stop, face\nthem and press [B]. Press [A]\nor [B] to scroll ahead. You\ncan talk to some other\ncharacters by facing them\nand pressing [B].") };
static const u8 dialog_text_DIALOG_001[] = { _("Watch out! If you wander\naround here, you're liable\nto be " "plastered" " by a\nwater bomb!\nThose enemy Bob-ombs love\nto fight, and they're\nalways finding ways to\nattack.\nThis meadow has become\na battlefield ever since\nthe Big Bob-omb got his\npaws on the Power Star.\nCan you recover the Star\nfor us? Cross the bridge\nand go left up the path\nto find the Big Bob-omb.\nPlease come back to see\nme after you've retrieved\nthe Power Star!") };
static const u8 dialog_text_DIALOG_002[] = { _("Hey, you! It's dangerous\nahead, so listen up! Take\nmy advice.\n\nCross the two\nbridges ahead, then\nwatch for falling\nwater bombs.\nThe Big Bob-omb at the\ntop of the mountain is\nvery powerful--don't let\nhim grab you!\nWe're Bob-omb Buddies,\nand we're on your side.\nYou can talk to us\nwhenever you'd like to!") };
static const u8 dialog_text_DIALOG_003[] = { _("Thank you, Mario! The Big\nBob-omb is nothing but a\nbig dud now! But the\nbattle for the castle has\njust begun.\nOther enemies are holding\nthe other Power Stars. If\nyou recover more Stars,\nyou can open new doors\nthat lead to new worlds!\nMy Bob-omb Buddies are\nwaiting for you. Be sure\nto talk to them--they'll\nset up cannons for you.") };
static const u8 dialog_text_DIALOG_004[] = { _("We're peace-loving\nBob-ombs, so we don't use\ncannons.\nBut if you'd like\nto blast off, we don't\nmind. Help yourself.\nWe'll prepare all of the\ncannons in this course for\nyou to use. Bon Voyage!") };
static const u8 dialog_text_DIALOG_005[] = { _("Hey, Mario! Is it true\nthat you beat the Big\nBob-omb? Cool!\nYou must be strong. And\npretty fast. So, how fast\nare you, anyway?\nFast enough to beat me...\nKoopa the Quick? I don't\nthink so. Just try me.\nHow about a race to the\nmountaintop, where the\nBig Bob-omb was?\nWhaddya say? When I say\n『Go,』 let the race begin!\n\nReady....\n\n//Go!////Don't Go") };
static const u8 dialog_text_DIALOG_006[] = { _("Hey!!! Don't try to " "scam\nME. " "You've gotta run\nthe whole course.\nLater. Look me up when\nyou want to race for\nreal.") };
static const u8 dialog_text_DIALOG_007[] = { _("Hufff...fff...pufff...\nWhoa! You...really...are...\nfast! A human blur!\nHere you go--you've won\nit, fair and square!") };
static const u8 dialog_text_DIALOG_008[] = { _("BEWARE OF CHAIN CHOMP\nExtreme Danger!\nGet close and press [C]^\nfor a better look.\nScary, huh?\nSee the Red Coin on top\nof the stake?\n\nWhen you collect eight of\nthem, a Power Star will\nappear in the meadow\nacross the bridge.") };
static const u8 dialog_text_DIALOG_009[] = { _("Long time, no see! Wow,\nhave you gotten fast!\nHave you been training\non the sly, or is it the\npower of the Stars?\nI've been feeling down\nabout losing the last\nrace. This is my home\ncourse--how about a\nrematch?\nThe goal is in\nWindswept Valley.\nReady?\n\n//Go//// Don't Go") };
static const u8 dialog_text_DIALOG_010[] = { _("You've stepped on the\nWing Cap Switch. Wearing\nthe Wing Cap, you can\nsoar through the sky.\nNow Wing Caps will pop\nout of all the red blocks\nyou find.\n\nWould you like to Save?\n\n//Yes////No") };
static const u8 dialog_text_DIALOG_011[] = { _("You've just stepped on\nthe Metal Cap Switch!\nThe Metal Cap makes\nMario invincible.\nNow Metal Caps will\npop out of all of the\ngreen blocks you find.\n\nWould you like to Save?\n\n//Yes////No") };
static const u8 dialog_text_DIALOG_012[] = { _("You've just stepped on\nthe Vanish Cap Switch.\nThe Vanish Cap makes\nMario disappear.\nNow Vanish Caps will pop\nfrom all of the blue\nblocks you find.\n\nWould you like to Save?\n\n//Yes////No") };
static const u8 dialog_text_DIALOG_013[] = { _("You've collected 100\ncoins! Mario gains more\npower from the castle.\nDo you want to Save?\n//Yes////No") };
static const u8 dialog_text_DIALOG_014[] = { _("Wow! Another Power Star!\nMario gains more courage\nfrom the power of the\ncastle.\nDo you want to Save?\n\n//You Bet//Not Now") };
static const u8 dialog_text_DIALOG_015[] = { _("You can punch enemies to\nknock them down. Press [A]\nto jump, [B] to punch.\nPress [A] then [B] to Kick.\nTo pick something up,\npress [B], too. To throw\nsomething you're holding,\npress [B] again.") };
static const u8 dialog_text_DIALOG_016[] = { _("Hop on the shiny shell and\nride wherever you want to\ngo! Shred those enemies!") };
static const u8 dialog_text_DIALOG_017[] = { _("I'm the Big Bob-omb, lord\nof all blasting matter,\nking of ka-booms the\nworld over!\nHow dare you scale my\nmountain? By what right\ndo you set foot on my\nimperial mountaintop?\nYou may have eluded my\nguards, but you'll never\nescape my grasp...\n\n...and you'll never take\naway my Power Star. I\nhereby challenge you,\nMario!\nIf you want the Star I\nhold, you must prove\nyourself in battle.\n\nCan you pick me up from\nthe back and hurl me to\nthis royal turf? I think\nthat you cannot!") };
static const u8 dialog_text_DIALOG_018[] = { _("I'm sleeping because...\n...I'm sleepy. I don't\nlike being disturbed.\nPlease walk quietly.") };
static const u8 dialog_text_DIALOG_019[] = { _("Shhh! Please walk\nquietly in the hallway!") };
static const u8 dialog_text_DIALOG_020[] = { _("Dear Mario:\nPlease come to the\ncastle. I've baked\na cake for you.\nYours truly--\nPrincess Toadstool") };
static const u8 dialog_text_DIALOG_021[] = { _("Welcome.\nNo one's home!\nNow " "scram--" "\nand don't come back!\nGwa ha ha!") };
static const u8 dialog_text_DIALOG_022[] = { _("You need a key to open\nthis door.") };
static const u8 dialog_text_DIALOG_023[] = { _("This key doesn't fit!\nMaybe it's for the\nbasement...") };
static const u8 dialog_text_DIALOG_024[] = { _("You need Star power to\nopen this door. Recover a\nPower Star from an enemy\ninside one of the castle's\npaintings.") };
static const u8 dialog_text_DIALOG_025[] = { _("It takes the power of\n3 Stars to open this\ndoor. You need [%] more\nStars.") };
static const u8 dialog_text_DIALOG_026[] = { _("It takes the power of\n8 Stars to open this\ndoor. You need [%] more\nStars.") };
static const u8 dialog_text_DIALOG_027[] = { _("It takes the power of\n30 Stars to open this\ndoor. You need [%] more\nStars.") };
static const u8 dialog_text_DIALOG_028[] = { _("It takes the power of\n50 Stars to open this\ndoor. You need [%] more\nStars.") };
static const u8 dialog_text_DIALOG_029[] = { _("To open the door that\nleads to the 『endless』\nstairs, you need 70\nStars.\nBwa ha ha!") };
static const u8 dialog_text_DIALOG_030[] = { _("Hello! The Lakitu Bros.,\ncutting in with a live\nupdate on Mario's\nprogress. He's about to\nlearn a technique for\nsneaking up on enemies.\nThe trick is this: He has\nto walk very slowly in\norder to walk quietly.\n\n\n\nAnd wrapping up filming\ntechniques reported on\nearlier, you can take a\nlook around using [C]> and\n[C]<. Press [C]| to view the\naction from a distance.\nWhen you can't move the\ncamera any farther, the\nbuzzer will sound. This is\nthe Lakitu Bros.,\nsigning off.") };
static const u8 dialog_text_DIALOG_031[] = { _("No way! You beat me...\nagain!! And I just spent\nmy entire savings on\nthese new Koopa\nMach 1 Sprint shoes!\nHere, I guess I have to\nhand over this Star to\nthe winner of the race.\nCongrats, Mario!") };
static const u8 dialog_text_DIALOG_032[] = { _("If you get the Wing Cap,\nyou can fly! Put the cap\non, then do a Triple\nJump--jump three times\nin a row--to take off.\nYou can fly even higher\nif you blast out of a\ncannon wearing the\nWing Cap!\n\nUse the [C] Buttons to look\naround while flying, and\npress [Z] to land.") };
static const u8 dialog_text_DIALOG_033[] = { _("Ciao! You've reached\nPrincess Toadstool's\ncastle via a warp pipe.\nUsing the controller is a\npiece of cake. Press [A] to\njump and [B] to attack.\nPress [B] to read signs,\ntoo. Use the Control Stick\nin the center of the\ncontroller to move Mario\naround. Now, head for\nthe castle.") };
static const u8 dialog_text_DIALOG_034[] = { _("Good afternoon. The\nLakitu Bros., here,\nreporting live from just\noutside the Princess's\ncastle.\n\nMario has just arrived\non the scene, and we'll\nbe filming the action live\nas he enters the castle\nand pursues the missing\nPower Stars.\nAs seasoned cameramen,\nwe'll be shooting from the\nrecommended angle, but\nyou can change the\ncamera angle by pressing\nthe [C] Buttons.\nIf we can't adjust the\nview any further, we'll\nbuzz. To take a look at\nthe surroundings, stop\nand press [C]^.\n\nPress [A] to resume play.\nSwitch camera modes with\nthe [R] Button. Signs along\nthe way will review these\ninstructions.\n\nFor now, reporting live,\nthis has been the\nLakitu Bros.") };
static const u8 dialog_text_DIALOG_035[] = { _("There are four camera, or\n『[C],』 Buttons. Press [C]^\nto look around using the\nControl Stick.\n\nYou'll usually see Mario\nthrough Lakitu's camera.\nIt is the camera\nrecommended for normal\nplay.\nYou can change angles by\npressing [C]>. If you press\n[R], the view switches to\nMario's camera, which\nis directly behind him.\nPress [R] again to return\nto Lakitu's camera. Press\n[C]| to see Mario from\nafar, using either\nLakitu's or Mario's view.") };
static const u8 dialog_text_DIALOG_036[] = { _("OBSERVATION PLATFORM\nPress [C]^ to take a look\naround. Don't miss\nanything!\n\nPress [R] to switch to\nMario's camera. It\nalways follows Mario.\nPress [R] again to switch\nto Lakitu's camera.\nPause the game and\nswitch the mode to 『fix』\nthe camera in place while\nholding [R]. Give it a try!") };
static const u8 dialog_text_DIALOG_037[] = { _("I win! You lose!\nHa ha ha ha!\nYou're no slouch, but I'm\na better sledder!\nBetter luck next time!") };
static const u8 dialog_text_DIALOG_038[] = { _("Reacting to the Star\npower, the door slowly\nopens.") };
static const u8 dialog_text_DIALOG_039[] = { _("No visitors allowed,\nby decree of\nthe Big Bob-omb\n\nI shall never surrender my\nStars, for they hold the\npower of the castle in\ntheir glow.\nThey were a gift from\nBowser, the Koopa King\nhimself, and they lie well\nhidden within my realm.\nNot a whisper of their\nwhereabouts shall leave\nmy lips. Oh, all right,\nperhaps one hint:\nHeed the Star names at\nthe beginning of the\ncourse.\n//--The Big Bob-omb") };
static const u8 dialog_text_DIALOG_040[] = { _("Warning!\nCold, Cold Crevasse\nBelow!") };
static const u8 dialog_text_DIALOG_041[] = { _("I win! You lose!\nHa ha ha!\n\nThat's what you get for\nmessin' with Koopa the\nQuick.\nBetter luck next time!") };
static const u8 dialog_text_DIALOG_042[] = { _("Caution! Narrow Bridge!\nCross slowly!\n\n\nYou can jump to the edge\nof the cliff and hang on,\nand you can climb off the\nedge if you move slowly.\nWhen you want to let go,\neither press [Z] or press\nthe Control Stick in the\ndirection of Mario's back.\nTo climb up, press Up on\nthe Control Stick. To\nscurry up quickly, press\nthe [A] Button.") };
static const u8 dialog_text_DIALOG_043[] = { _("If you jump and hold the\n[A] Button, you can hang on\nto some objects overhead.\nIt's the same as grabbing\na flying bird!") };
static const u8 dialog_text_DIALOG_044[] = { _("Whooo's there? Whooo\nwoke me up? It's still\ndaylight--I should be\nsleeping!\n\nHey, as long as I'm\nawake, why not take a\nshort flight with me?\nPress and hold [A] to grab\non. Release [A] to let go.\nI'll take you wherever\nyou want to go, as long\nas my wings hold out.\nWatch my shadow, and\ngrab on.") };
static const u8 dialog_text_DIALOG_045[] = { _("Whew! I'm just about\nflapped out. You should\nlay off the pasta, Mario!\nThat's it for now. Press\n[A] to let go. Okay,\nbye byyyyyyeeee!") };
static const u8 dialog_text_DIALOG_046[] = { _("You have to master three\nimportant jumping\ntechniques.\nFirst try the Triple Jump.\n\nRun fast, then jump three\ntimes, one, two, three.\nIf you time the jumps\nright, you'll hop, skip,\nthen jump really high.\nNext, go for distance\nwith the Long Jump. Run,\npress [Z] to crouch then [A]\nto jump really far.\n\nTo do the Wall Kick, press\n[A] to jump at a wall, then\njump again when you hit\nthe wall.\n\nGot that? Triple Jump,\nLong Jump, Wall Kick.\nPractice, practice,\npractice. You don't stand\na chance without them.") };
static const u8 dialog_text_DIALOG_047[] = { _("Hi! I'll prepare the\ncannon for you!") };
static const u8 dialog_text_DIALOG_048[] = { _("Snow Mountain Summit\nWatch for slippery\nconditions! Please enter\nthe cottage first.") };
static const u8 dialog_text_DIALOG_049[] = { _("Remember that tricky Wall\nKick jump? It's a\ntechnique you'll have to\nmaster in order to reach\nhigh places.\nUse it to jump from wall\nto wall. Press the\nControl Stick in the\ndirection you want to\nbounce to gain momentum.\nPractice makes perfect!") };
static const u8 dialog_text_DIALOG_050[] = { _("Hold [Z] to crouch and\nslide down a slope.\nOr press [Z] while in the\nair to Pound the Ground!\nIf you stop, crouch, then\njump, you'll do a\nBackward Somersault!\nGot that?\nThere's more. Crouch and\nthen jump to do a\nLong Jump! Or crouch and\nwalk to...never mind.") };
static const u8 dialog_text_DIALOG_051[] = { _("Climbing's easy! When you\njump at trees, poles or\npillars, you'll grab them\nautomatically. Press [A] to\njump off backward.\n\nTo rotate around the\nobject, press Right or\nLeft on the Control Stick.\nWhen you reach the top,\npress Up to do a\nhandstand!\nJump off from the\nhandstand for a high,\nstylin' dismount.") };
static const u8 dialog_text_DIALOG_052[] = { _("Stop and press [Z] to\ncrouch, then press [A]\nto do a high, Backward\nSomersault!\n\nTo perform a Side\nSomersault, run, do a\nsharp U-turn and jump.\nYou can catch lots of\nair with both jumps.") };
static const u8 dialog_text_DIALOG_053[] = { _("Sometimes, if you pass\nthrough a coin ring or\nfind a secret point in a\ncourse, a red number will\nappear.\nIf you trigger five red\nnumbers, a secret Star\nwill show up.") };
static const u8 dialog_text_DIALOG_054[] = { _("Welcome to the snow\nslide! Hop on! To speed\nup, press forward on the\nControl Stick. To slow\ndown, pull back.") };
static const u8 dialog_text_DIALOG_055[] = { _("Hey-ey, Mario, buddy,\nhowzit goin'? Step right\nup. You look like a fast\nsleddin' kind of guy.\nI know speed when I see\nit, yes siree--I'm the\nworld champion sledder,\nyou know. Whaddya say?\nHow about a race?\nReady...\n\n//Go//// Don't Go") };
static const u8 dialog_text_DIALOG_056[] = { _("You brrrr-oke my record!\nUnbelievable! I knew\nthat you were the coolest.\nNow you've proven\nthat you're also the\nfastest!\nI can't award you a gold\nmedal, but here, take this\nStar instead. You've\nearned it!") };
static const u8 dialog_text_DIALOG_057[] = { _("Egad! My baby!! Have you\nseen my baby??? She's\nthe most precious baby in\nthe whole wide world.\n(They say she has my\nbeak...) I just can't\nremember where I left\nher.\nLet's see...I stopped\nfor herring and ice cubes,\nthen I...oohh! I just\ndon't know!") };
static const u8 dialog_text_DIALOG_058[] = { _("You found my precious,\nprecious baby! Where\nhave you been? How can\nI ever thank you, Mario?\nOh, I do have this...\n...Star. Here, take it\nwith my eternal\ngratitude.") };
static const u8 dialog_text_DIALOG_059[] = { _("That's not my baby! She\nlooks nothing like me!\nHer parents must be\nworried sick!") };
static const u8 dialog_text_DIALOG_060[] = { _("ATTENTION!\nRead Before Diving In!\n\n\nIf you stay under the\nwater for too long, you'll\nrun out of oxygen.\n\nReturn to the surface for\nair or find an air bubble\nor coins to breathe while\nunderwater.\nPress [A] to swim. Hold [A]\nto swim slow and steady.\nTap [A] with smooth timing\nto gain speed.\nPress Up on the\nControl Stick and press [A]\nto dive.\n\nPress Down on the Control\nStick and press [A] to\nreturn to the surface.\n\nHold Down and press [A]\nwhile on the surface near\nthe edge of the water to\njump out.") };
static const u8 dialog_text_DIALOG_061[] = { _("BRRR! Frostbite Danger!\nDo not swim here.\nI'm serious.\n/--The Penguin") };
static const u8 dialog_text_DIALOG_062[] = { _("Hidden inside the green\nblock is the amazing\nMetal Cap.\nWearing it, you won't\ncatch fire or be hurt\nby enemy attacks.\nYou don't even have to\nbreathe while wearing it.\n\nThe only problem:\n" "You can't swim in it.") };
static const u8 dialog_text_DIALOG_063[] = { _("The Vanish Cap is inside\nthe blue block. Mr. I.\nwill be surprised, since\nyou'll be invisible when\nyou wear it!\nEven the Big Boo will be\nfooled--and you can walk\nthrough secret walls, too.") };
static const u8 dialog_text_DIALOG_064[] = { _("When you put on the Wing\nCap that comes from a\nred block, do the Triple\nJump to soar high into\nthe sky.\nUse the Control Stick to\nguide Mario. Pull back to\nto fly up, press forward\nto nose down, and press [Z]\nto land.") };
static const u8 dialog_text_DIALOG_065[] = { _("Swimming Lessons!\nTap [A] to do the breast\nstroke. If you time the\ntaps right, you'll swim\nfast.\n\nPress and hold [A] to do a\nslow, steady flutter kick.\nPress Up on the Control\nStick to dive, and pull\nback on the stick to head\nfor the surface.\nTo jump out of the water,\nhold Down on the Control\nStick, then press [A].\nEasy as pie, right?\n\n\nBut remember:\nMario can't breathe under\nthe water! Return to the\nsurface for air when the\nPower Meter runs low.\n\nAnd one last thing: You\ncan't open doors that\nare underwater.") };
static const u8 dialog_text_DIALOG_066[] = { _("Mario, it's Peach!\nPlease be careful! Bowser\nis so wicked! He will try\nto burn you with his\nhorrible flame breath.\nRun around behind and\ngrab him by the tail with\nthe [B] Button. Once you\ngrab hold, swing him\naround in great circles.\nRotate the Control Stick\nto go faster and faster.\nThe faster you swing him,\nthe farther he'll fly.\n\nUse the [C] Buttons to look\naround, Mario. You have\nto throw Bowser into one\nof the bombs in the four\ncorners.\nAim well, then press [B]\nagain to launch Bowser.\nGood luck, Mario! Our\nfate is in your hands.") };
static const u8 dialog_text_DIALOG_067[] = { _("Tough luck, Mario!\nPrincess Toadstool isn't\nhere...Gwa ha ha!! Go\nahead--just try to grab\nme by the tail!\nYou'll never be able to\nswing ME around! A wimp\nlike you won't throw me\nout of here! Never! Ha!") };
static const u8 dialog_text_DIALOG_068[] = { _("It's Lethal Lava Land!\nIf you catch fire or fall\ninto a pool of flames,\nyou'll be hopping mad, but\ndon't lose your cool.\nYou can still control\nMario--just try to keep\ncalm!") };
static const u8 dialog_text_DIALOG_069[] = { _("Sometimes you'll bump into\ninvisible walls at the\nedges of the painting\nworlds. If you hit a wall\nwhile flying, you'll bounce\nback.") };
static const u8 dialog_text_DIALOG_070[] = { _("You can return to the\ncastle's main hall at any\ntime from the painting\nworlds where the enemies\nlive.\nJust stop, stand still,\npress Start to pause the\ngame, then select\n『Exit Course.』\n\nYou don't have to collect\nall Power Stars in one\ncourse before going on to\nthe next.\n\nReturn later, when you're\nmore experienced, to pick\nup difficult ones.\n\n\nWhenever you find a Star,\na hint for finding the\nnext one will appear on\nthe course's start screen.\n\nYou can, however, collect\nany of the remaining\nStars next. You don't\nhave to recover the one\ndescribed by the hint.") };
static const u8 dialog_text_DIALOG_071[] = { _("Danger Ahead!\nBeware of the strange\ncloud! Don't inhale!\nIf you feel faint, run for\nhigher ground and fresh\nair!\nCircle: Shelter\nArrow: Entrance-Exit") };
static const u8 dialog_text_DIALOG_072[] = { _("High winds ahead!\nPull your Cap down tight.\nIf it blows off, you'll\nhave to find it on this\nmountain.") };
static const u8 dialog_text_DIALOG_073[] = { _("Aarrgh! Ahoy, matey. I\nhave sunken treasure,\nhere, I do.\n\nBut to pluck the plunder,\nyou must open the\nTreasure Chests in the\nright order.\nWhat order is that,\nye say?\n\n\nI'll never tell!\n\n//--The Cap'n") };
static const u8 dialog_text_DIALOG_074[] = { _("You can grab on to the\nedge of a cliff or ledge\nwith your fingertips and\nhang down from it.\n\nTo drop from the edge,\neither press the Control\nStick in the direction of\nMario's back or press the\n[Z] Button.\nTo get up onto the ledge,\neither press Up on the\nControl Stick or press [A]\nas soon as you grab the\nledge to climb up quickly.") };
static const u8 dialog_text_DIALOG_075[] = { _("Mario!! My castle is in\ngreat peril. I know that\nBowser is the cause...and\nI know that only you can\nstop him!\nThe doors in the castle\nthat have been sealed by\nBowser can be opened only\nwith Star Power.\n\nBut there are secret\npaths in the castle,\npaths that Bowser hasn't\nfound.\n\nOne of those paths is in\nthis room, and it holds\none of the castle's Secret\nStars!\n\nFind that Secret Star,\nMario! It will help you\non your quest. Please,\nMario, you have to\nhelp us!\nRetrieve all of the\nPower Stars in the castle\nand free us from this\nawful prison!\nPlease!") };
static const u8 dialog_text_DIALOG_076[] = { _("Thanks to the power of\nthe Stars, life is\nreturning to the castle.\nPlease, Mario, you have\nto give Bowser the boot!\n\nHere, let me tell you a\nlittle something about the\ncastle. In the room with\nthe mirrors, look carefully\nfor anything that's not\nreflected in the mirror.\nAnd when you go to the\nwater town, you can flood\nit with a high jump into\nthe painting. Oh, by the\nway, look what I found!") };
static const u8 dialog_text_DIALOG_077[] = { _("It is decreed that one\nshall pound the pillars.") };
static const u8 dialog_text_DIALOG_078[] = { _("Break open the Blue Coin\nBlock by Pounding the\nGround with the [Z] Button.\nOne Blue Coin is worth\nfive Yellow Coins.\nBut you have to hurry!\nThe coins will disappear\nif you're not quick to\ncollect them! Too bad.") };
static const u8 dialog_text_DIALOG_079[] = { _("Owwwuu! Let me go!\nUukee-kee! I was only\nteasing! Can't you take\na joke?\nI'll tell you what, let's\ntrade. If you let me go,\nI'll give you something\nreally good.\nSo, how about it?\n\n//Free him/ Hold on") };
static const u8 dialog_text_DIALOG_080[] = { _("Eeeh hee hee hee!") };
static const u8 dialog_text_DIALOG_081[] = { _("The mystery is of Wet\nor Dry.\nAnd where does the\nsolution lie?\nThe city welcomes visitors\nwith the depth they bring\nas they enter.") };
static const u8 dialog_text_DIALOG_082[] = { _("Hold on to your hat! If\nyou lose it, you'll be\ninjured easily.\n\nIf you do lose your Cap,\nyou'll have to find it in\nthe course where you\nlost it.\nOh, boy, it's not looking\ngood for Peach. She's\nstill trapped somewhere\ninside the walls.\nPlease, Mario, you have\nto help her! Did you know\nthat there are enemy\nworlds inside the walls?\nYup. It's true. Bowser's\ntroops are there, too.\nOh, here, take this. I've\nbeen keeping it for you.") };
static const u8 dialog_text_DIALOG_083[] = { _("There's something strange\nabout that clock. As you\njump inside, watch the\nposition of the big hand.\nOh, look what I found!\nHere, Mario, catch!") };
static const u8 dialog_text_DIALOG_084[] = { _("Yeeoww! Unhand me,\nbrute! I'm late, so late,\nI must make haste!\nThis shiny thing? Mine!\nIt's mine. Finders,\nkeepers, losers...\nLate, late, late...\nOuch! Take it then! A\ngift from Bowser, it was.\nNow let me be! I have a\ndate! I cannot be late\nfor tea!") };
static const u8 dialog_text_DIALOG_085[] = { _("You don't stand a ghost\nof a chance in this house.\nIf you walk out of here,\nyou deserve...\n...a Ghoul Medal...") };
static const u8 dialog_text_DIALOG_086[] = { _("Running around in circles\nmakes some bad guys roll\ntheir eyes.") };
static const u8 dialog_text_DIALOG_087[] = { _("Santa Claus isn't the only\none who can go down a\nchimney! Come on in!\n/--Cabin Proprietor") };
static const u8 dialog_text_DIALOG_088[] = { _("Work Elevator\nFor those who get off\nhere: Grab the pole to the\nleft and slide carefully\ndown.") };
static const u8 dialog_text_DIALOG_089[] = { _("Both ways fraught with\ndanger! Watch your feet!\nThose who can't do the\nLong Jump, tsk, tsk. Make\nyour way to the right.\nRight: Work Elevator\n/// Cloudy Maze\nLeft: Black Hole\n///Underground Lake\n\nRed Circle: Elevator 2\n//// Underground Lake\nArrow: You are here") };
static const u8 dialog_text_DIALOG_090[] = { _("Bwa ha ha ha!\nYou've stepped right into\nmy trap, just as I knew\nyou would! I warn you,\n『Friend,』 watch your\nstep!") };
static const u8 dialog_text_DIALOG_091[] = { _("Danger!\nStrong Gusts!\nBut the wind makes a\ncomfy ride.") };
static const u8 dialog_text_DIALOG_092[] = { _("Pestering me again, are\nyou, Mario? Can't you see\nthat I'm having a merry\nlittle time, making\nmischief with my minions?\nNow, return those Stars!\nMy troops in the walls\nneed them! Bwa ha ha!") };
static const u8 dialog_text_DIALOG_093[] = { _("Mario! You again! Well\nthat's just fine--I've\nbeen looking for something\nto fry with my fire\nbreath!\nYour Star Power is\nuseless against me!\nYour friends are all\ntrapped within the\nwalls...\nAnd you'll never see the\nPrincess again!\nBwa ha ha ha!") };
static const u8 dialog_text_DIALOG_094[] = { _("Get a good run up the\nslope! Do you remember\nthe Long Jump? Run, press\n[Z], then jump!") };
static const u8 dialog_text_DIALOG_095[] = { _("To read a sign, stand in\nfront of it and press [B],\nlike you did just now.\n\nWhen you want to talk to\na Koopa Troopa or other\nanimal, stand right in\nfront of it.\nPlease recover the Stars\nthat were stolen by\nBowser in this course.") };
static const u8 dialog_text_DIALOG_096[] = { _("The path is narrow here.\nEasy does it! No one is\nallowed on top of the\nmountain!\nAnd if you know what's\ngood for you, you won't\nwake anyone who's\nsleeping!\nMove slowly,\ntread lightly.") };
static const u8 dialog_text_DIALOG_097[] = { _("Don't be a pushover!\nIf anyone tries to shove\nyou around, push back!\nIt's one-on-one, with a\nfiery finish for the loser!") };
static const u8 dialog_text_DIALOG_098[] = { _("Come on in here...\n...heh, heh, heh...") };
static const u8 dialog_text_DIALOG_099[] = { _("Eh he he...\nYou're mine, now, hee hee!\nI'll pass right through\nthis wall. Can you do\nthat? Heh, heh, heh!") };
static const u8 dialog_text_DIALOG_100[] = { _("Ukkiki...Wakkiki...kee kee!\nHa! I snagged it!\nIt's mine! Heeheeheeee!") };
static const u8 dialog_text_DIALOG_101[] = { _("Ackk! Let...go...\nYou're...choking...me...\nCough...I've been framed!\nThis Cap? Oh, all right,\ntake it. It's a cool Cap,\nbut I'll give it back.\nI think it looks better on\nme than it does on you,\nthough! Eeeee! Kee keee!") };
static const u8 dialog_text_DIALOG_102[] = { _("Pssst! The Boos are super\nshy. If you look them\nin the eyes, they fade\naway, but if you turn\nyour back, they reappear.\nIt's no use trying to hit\nthem when they're fading\naway. Instead, sneak up\nbehind them and punch.") };
static const u8 dialog_text_DIALOG_103[] = { _("Upon four towers\none must alight...\nThen at the peak\nshall shine the light...") };
static const u8 dialog_text_DIALOG_104[] = { _("The shadowy star in front\nof you is a 『Star\nMarker.』 When you collect\nall 8 Red Coins, the Star\nwill appear here.") };
static const u8 dialog_text_DIALOG_105[] = { _("Ready for blastoff! Come\non, hop into the cannon!\n\nYou can reach the Star on\nthe floating island by\nusing the four cannons.\nUse the Control Stick to\naim, then press [A] to fire.\n\nIf you're handy, you can\ngrab on to trees or poles\nto land.") };
static const u8 dialog_text_DIALOG_106[] = { _("Ready for blastoff! Come\non, hop into the cannon!") };
static const u8 dialog_text_DIALOG_107[] = { _("Ghosts...\n...don't...\n...DIE!\nHeh, heh, heh!\nCan you get out of here...\n...alive?") };
static const u8 dialog_text_DIALOG_108[] = { _("Boooooo-m! Here comes\nthe master of mischief,\nthe tower of terror,\nthe Big Boo!\nKa ha ha ha...") };
static const u8 dialog_text_DIALOG_109[] = { _("Ooooo Nooooo!\nTalk about out-of-body\nexperiences--my body\nhas melted away!\nHave you run in to any\nheadhunters lately??\nI could sure use a new\nbody!\nBrrr! My face might\nfreeze like this!") };
static const u8 dialog_text_DIALOG_110[] = { _("I need a good head on my\nshoulders. Do you know of\nanybody in need of a good\nbody? Please! I'll follow\nyou if you do!") };
static const u8 dialog_text_DIALOG_111[] = { _("Perfect! What a great\nnew body! Here--this is a\npresent for you. It's sure\nto warm you up.") };
static const u8 dialog_text_DIALOG_112[] = { _("Collect as many coins as\npossible! They'll refill\nyour Power Meter.\n\nYou can check to see how\nmany coins you've\ncollected in each of the\n15 enemy worlds.\nYou can also recover\npower by touching the\nSpinning Heart.\n\nThe faster you run\nthrough the heart, the\nmore power you'll recover.") };
static const u8 dialog_text_DIALOG_113[] = { _("There are special Caps in\nthe red, green and blue\nblocks. Step on the\nswitches in the hidden\ncourses to activate the\nCap Blocks.") };
static const u8 dialog_text_DIALOG_114[] = { _("It makes me so mad! We\nbuild your houses, your\ncastles. We pave your\nroads, and still you\nwalk all over us.\nDo you ever say thank\nyou? No! Well, you're not\ngoing to wipe your feet\non me! I think I'll crush\nyou just for fun!\nDo you have a problem\nwith that? Just try to\npound me, wimp! Ha!") };
static const u8 dialog_text_DIALOG_115[] = { _("No! Crushed again!\nI'm just a stepping stone,\nafter all. I won't gravel,\ner, grovel. Here, you win.\nTake this with you!") };
static const u8 dialog_text_DIALOG_116[] = { _("Whaaa....Whaaat?\nCan it be that a\npipsqueak like you has\ndefused the Bob-omb\nking????\nYou might be fast enough\nto ground me, but you'll\nhave to pick up the pace\nif you want to take King\nBowser by the tail.\nMethinks my troops could\nlearn a lesson from you!\nHere is your Star, as I\npromised, Mario.\n\nIf you want to see me\nagain, select this Star\nfrom the menu. For now,\nfarewell.") };
static const u8 dialog_text_DIALOG_117[] = { _("Who...walk...here?\nWho...break...seal?\nWake..ancient..ones?\nWe no like light...\nRrrrummbbble...\nWe no like...intruders!\nNow battle...\n...hand...\n...to...\n...hand!") };
static const u8 dialog_text_DIALOG_118[] = { _("Grrrrumbbble!\nWhat...happen?\nWe...crushed like pebble.\nYou so strong!\nYou rule ancient pyramid!\nFor today...\nNow, take Star of Power.\nWe...sleep...darkness.") };
static const u8 dialog_text_DIALOG_119[] = { _("Grrr! I was a bit\ncareless. This is not as I\nhad planned...but I still\nhold the power of the\nStars, and I still have\nPeach.\nBwa ha ha! You'll get no\nmore Stars from me! I'm\nnot finished with you yet,\nbut I'll let you go for\nnow. You'll pay for this...\nlater!") };
static const u8 dialog_text_DIALOG_120[] = { _("Ooowaah! Can it be that\nI've lost??? The power of\nthe Stars has failed me...\nthis time.\nConsider this a draw.\nNext time, I'll be in\nperfect condition.\n\nNow, if you want to see\nyour precious Princess,\ncome to the top of the\ntower.\nI'll be waiting!\nGwa ha ha ha!") };
static const u8 dialog_text_DIALOG_121[] = { _("Nooo! It can't be!\nYou've really beaten me,\nMario?!! I gave those\ntroops power, but now\nit's fading away!\nArrgghh! I can see peace\nreturning to the world! I\ncan't stand it! Hmmm...\nIt's not over yet...\n\nC'mon troops! Let's watch\nthe ending together!\nBwa ha ha!") };
static const u8 dialog_text_DIALOG_122[] = { _("The Black Hole\nRight: Work Elevator\n/// Cloudy Maze\nLeft: Underground Lake") };
static const u8 dialog_text_DIALOG_123[] = { _("Metal Cavern\nRight: To Waterfall\nLeft: Metal Cap Switch") };
static const u8 dialog_text_DIALOG_124[] = { _("Work Elevator\nDanger!!\nRead instructions\nthoroughly!\nElevator continues in the\ndirection of the arrow\nactivated.") };
static const u8 dialog_text_DIALOG_125[] = { _("Hazy Maze-Exit\nDanger! Closed.\nTurn back now.") };
static const u8 dialog_text_DIALOG_126[] = { _("Up: Black Hole\nRight: Work Elevator\n/// Hazy Maze") };
static const u8 dialog_text_DIALOG_127[] = { _("Underground Lake\nRight: Metal Cave\nLeft: Abandoned Mine\n///(Closed)\nA gentle sea dragon lives\nhere. Pound on his back to\nmake him lower his head.\nDon't become his lunch.") };
static const u8 dialog_text_DIALOG_128[] = { _("You must fight with\nhonor! It is against the\nroyal rules to throw the\nking out of the ring!") };
static const u8 dialog_text_DIALOG_129[] = { _("Welcome to the Vanish\nCap Switch Course! All of\nthe blue blocks you find\nwill become solid once you\nstep on the Cap Switch.\nYou'll disappear when you\nput on the Vanish Cap, so\nyou'll be able to elude\nenemies and walk through\nmany things. Try it out!") };
static const u8 dialog_text_DIALOG_130[] = { _("Welcome to the Metal Cap\nSwitch Course! Once you\nstep on the Cap Switch,\nthe green blocks will\nbecome solid.\nWhen you turn your body\ninto metal with the Metal\nCap, you can walk\nunderwater! Try it!") };
static const u8 dialog_text_DIALOG_131[] = { _("Welcome to the Wing Cap\nCourse! Step on the red\nswitch at the top of the\ntower, in the center of\nthe rainbow ring.\nWhen you trigger the\nswitch, all of the red\nblocks you find will\nbecome solid.\n\nTry out the Wing Cap! Do\nthe Triple Jump to take\noff and press [Z] to land.\n\n\nPull back on the Control\nStick to go up and push\nforward to nose down,\njust as you would when\nflying an airplane.") };
static const u8 dialog_text_DIALOG_132[] = { _("Whoa, Mario, pal, you\naren't trying to cheat,\nare you? Shortcuts aren't\nallowed.\nNow, I know that you\nknow better. You're\ndisqualified! Next time,\nplay fair!") };
static const u8 dialog_text_DIALOG_133[] = { _("Am I glad to see you! The\nPrincess...and I...and,\nwell, everybody...we're all\ntrapped inside the castle\nwalls.\n\nBowser has stolen the\ncastle's Stars, and he's\nusing their power to\ncreate his own world in\nthe paintings and walls.\n\nPlease recover the Power\nStars! As you find them,\nyou can use their power\nto open the doors that\nBowser has sealed.\n\nThere are four rooms on\nthe first floor. Start in\nthe one with the painting\nof Bob-omb inside. It's\nthe only room that Bowser\nhasn't sealed.\nWhen you collect eight\nPower Stars, you'll be\nable to open the door\nwith the big star. The\nPrincess must be inside!") };
static const u8 dialog_text_DIALOG_134[] = { _("The names of the Stars\nare also hints for\nfinding them. They are\ndisplayed at the beginning\nof each course.\nYou can collect the Stars\nin any order. You won't\nfind some Stars, enemies\nor items unless you select\na specific Star.\nAfter you collect some\nStars, you can try\nanother course.\nWe're all waiting for\nyour help!") };
static const u8 dialog_text_DIALOG_135[] = { _("It was Bowser who stole\nthe Stars. I saw him with\nmy own eyes!\n\n\nHe's hidden six Stars in\neach course, but you\nwon't find all of them in\nsome courses until you\npress the Cap Switches.\nThe Stars you've found\nwill show on each course's\nstarting screen.\n\n\nIf you want to see some\nof the enemies you've\nalready defeated, select\nthe Stars you recovered\nfrom them.") };
static const u8 dialog_text_DIALOG_136[] = { _("Wow! You've already\nrecovered that many\nStars? Way to go, Mario!\nI'll bet you'll have us out\nof here in no time!\n\nBe careful, though.\nBowser and his band\nwrote the book on 『bad.』\nTake my advice: When you\nneed to recover from\ninjuries, collect coins.\nYellow Coins refill one\npiece of the Power Meter,\nRed Coins refill two\npieces, and Blue Coins\nrefill five.\n\nTo make Blue Coins\nappear, pound on Blue\nCoin Blocks.\n\n\n\nAlso, if you fall from\nhigh places, you'll\nminimize damage if you\nPound the Ground as you\nland.") };
static const u8 dialog_text_DIALOG_137[] = { _("Thanks, Mario! The castle\nis recovering its energy\nas you retrieve Power\nStars, and you've chased\nBowser right out of here,\non to some area ahead.\nOh, by the by, are you\ncollecting coins? Special\nStars appear when you\ncollect 100 coins in each\nof the 15 courses!") };
static const u8 dialog_text_DIALOG_138[] = { _("Down: Underground Lake\nLeft: Black Hole\nRight: Hazy Maze (Closed)") };
static const u8 dialog_text_DIALOG_139[] = { _("Above: Automatic Elevator\nElevator begins\nautomatically and follows\npre-set course.\nIt disappears\nautomatically, too.") };
static const u8 dialog_text_DIALOG_140[] = { _("Elevator Area\nRight: Hazy Maze\n/// Entrance\nLeft: Black Hole\n///Elevator 1\nArrow: You are here") };
static const u8 dialog_text_DIALOG_141[] = { _("You've recovered one of\nthe stolen Power Stars!\nNow you can open some of\nthe sealed doors in the\ncastle.\nTry the Princess's room\non the second floor and\nthe room with the\npainting of Whomp's\nFortress on Floor 1.\nBowser's troops are still\ngaining power, so you\ncan't give up. Save us,\nMario! Keep searching for\nStars!") };
static const u8 dialog_text_DIALOG_142[] = { _("You've recovered three\nPower Stars! Now you can\nopen any door with a 3\non its star.\n\nYou can come and go from\nthe open courses as you\nplease. The enemies ahead\nare even meaner, so be\ncareful!") };
static const u8 dialog_text_DIALOG_143[] = { _("You've recovered eight of\nthe Power Stars! Now you\ncan open the door with\nthe big Star! But Bowser\nis just ahead...can you\nhear the Princess calling?") };
static const u8 dialog_text_DIALOG_144[] = { _("You've recovered 30\nPower Stars! Now you can\nopen the door with the\nbig Star! But before you\nmove on, how's it going\notherwise?\nDid you pound the two\ncolumns down? You didn't\nlose your hat, did you?\nIf you did, you'll have to\nstomp on the condor to\nget it back!\nThey say that Bowser has\nsneaked out of the sea\nand into the underground.\nHave you finally\ncornered him?") };
static const u8 dialog_text_DIALOG_145[] = { _("You've recovered 50\nPower Stars! Now you can\nopen the Star Door on the\nthird floor. Bowser's\nthere, you know.\n\nOh! You've found all of\nthe Cap Switches, haven't\nyou? Red, green and blue?\nThe Caps you get from the\ncolored blocks are really\nhelpful.\nHurry along, now. The\nthird floor is just ahead.") };
static const u8 dialog_text_DIALOG_146[] = { _("You've found 70 Power\nStars! The mystery of the\nendless stairs is solved,\nthanks to you--and is\nBowser ever upset! Now,\non to the final bout!") };
static const u8 dialog_text_DIALOG_147[] = { _("Are you using the Cap\nBlocks? You really should,\nyou know.\n\n\nTo make them solid so you\ncan break them, you have\nto press the colored Cap\nSwitches in the castle's\nhidden courses.\nYou'll find the hidden\ncourses only after\nregaining some of the\nPower Stars.\n\nThe Cap Blocks are a big\nhelp! Red for the Wing\nCap, green for the Metal\nCap, blue for the Vanish\nCap.") };
static const u8 dialog_text_DIALOG_148[] = { _("Snowman Mountain ahead.\nKeep out! And don't try\nthe Triple Jump over the\nice block shooter.\n\n\nIf you fall into the\nfreezing pond, your power\ndecreases quickly, and\nyou won't recover\nautomatically.\n//--The Snowman") };
static const u8 dialog_text_DIALOG_149[] = { _("Welcome to\nPrincess Toadstool's\nsecret slide!\nThere's a Star hidden\nhere that Bowser couldn't\nfind.\nWhen you slide, press\nforward to speed up,\npull back to slow down.\nIf you slide really\nfast, you'll win the Star!") };
static const u8 dialog_text_DIALOG_150[] = { _("Waaaa! You've flooded my\nhouse! Wh-why?? Look at\nthis mess! What am I\ngoing to do now?\n\nThe ceiling's ruined, the\nfloor is soaked...what to\ndo, what to do? Huff...\nhuff...it makes me so...\nMAD!!!\nEverything's been going\nwrong ever since I got\nthis Star...It's so shiny,\nbut it makes me feel...\nstrange...") };
static const u8 dialog_text_DIALOG_151[] = { _("I can't take this\nanymore! First you get\nme all wet, then you\nstomp on me!\nNow I'm really, really,\nREALLY mad!\nWaaaaaaaaaaaaaaaaa!!!") };
static const u8 dialog_text_DIALOG_152[] = { _("Owwch! Uncle! Uncle!\nOkay, I " "give" ". Take this\nStar!\nWhew! I feel better now.\nI don't really need it\nanymore, anyway--\nI can see the stars\nthrough my ceiling at\nnight.\nThey make me feel...\n...peaceful. Please, come\nback and visit anytime.") };
static const u8 dialog_text_DIALOG_153[] = { _("Hey! Who's there?\nWhat's climbing on me?\nIs it an ice ant?\nA snow flea?\nWhatever it is, it's\nbugging me! I think I'll\nblow it away!") };
static const u8 dialog_text_DIALOG_154[] = { _("Hold on to your hat! If\nyou lose it, you'll be\neasily injured. If you\nlose it, look for it in the\ncourse where you lost it.\nSpeaking of lost, the\nPrincess is still stuck in\nthe walls somewhere.\nPlease help, Mario!\n\nOh, you know that there\nare secret worlds in the\nwalls as well as in the\npaintings, right?") };
static const u8 dialog_text_DIALOG_155[] = { _("Thanks to the power of\nthe Stars, life is\nreturning to the castle.\nPlease, Mario, you have\nto give Bowser the boot!\n\nHere, let me tell you a\nlittle something about the\ncastle. In the room with\nthe mirrors, look carefully\nfor anything that's not\nreflected in the mirror.\nAnd when you go to the\nwater town, you can flood\nit with a high jump into\nthe painting.") };
static const u8 dialog_text_DIALOG_156[] = { _("The world inside the\nclock is so strange!\nWhen you jump inside,\nwatch the position of\nthe big hand!") };
static const u8 dialog_text_DIALOG_157[] = { _("Watch out! Don't let\nyourself be swallowed by\nquicksand.\n\n\nIf you sink into the sand,\nyou won't be able to\njump, and if your head\ngoes under, you'll be\nsmothered.\nThe dark areas are\nbottomless pits.") };
static const u8 dialog_text_DIALOG_158[] = { _("1. If you jump repeatedly\nand time it right, you'll\njump higher and higher.\nIf you run really fast and\ntime three jumps right,\nyou can do a Triple Jump.\n2. Jump into a solid wall,\nthen jump again when you\nhit the wall. You can\nbounce to a higher level\nusing this Wall Kick.") };
static const u8 dialog_text_DIALOG_159[] = { _("3. If you stop, press [Z]\nto crouch, then jump, you\ncan perform a Backward\nSomersault. To do a Long\nJump, run fast, press [Z],\nthen jump.") };
static const u8 dialog_text_DIALOG_160[] = { _("Press [B] while running\nfast to do a Body Slide\nattack. To stand while\nsliding, press [A] or [B].") };
static const u8 dialog_text_DIALOG_161[] = { _("Mario!!!\nIt that really you???\nIt has been so long since\nour last adventure!\nThey told me that I might\nsee you if I waited here,\nbut I'd just about given\nup hope!\nIs it true? Have you\nreally beaten Bowser? And\nrestored the Stars to the\ncastle?\nAnd saved the Princess?\nI knew you could do it!\nNow I have a very special\nmessage for you.\n『Thanks for playing Super\nMario 64! This is the\nend of the game, but not\nthe end of the fun." "\nWe want you to keep on\nplaying, so we have a\nlittle something for you.\nWe hope that you like it!\nEnjoy!!!" "』\n\nThe Super Mario 64 Team") };
static const u8 dialog_text_DIALOG_162[] = { _("No, no, no! Not you\nagain! I'm in a great\nhurry, can't you see?\n\nI've no time to squabble\nover Stars. Here, have it.\nI never meant to hide it\nfrom you...\nIt's just that I'm in such\na rush. That's it, that's\nall. Now, I must be off.\nOwww! Let me go!") };
static const u8 dialog_text_DIALOG_163[] = { _("Noooo! You've really\nbeaten me this time,\nMario! I can't stand\nlosing to you!\n\nMy troops...worthless!\nThey've turned over all\nthe Power Stars! What?!\nThere are 120 in all???\n\nAmazing! There were some\nin the castle that I\nmissed??!!\n\n\nNow I see peace\nreturning to the world...\nOooo! I really hate that!\nI can't watch--\nI'm outta here!\nJust you wait until next\ntime. Until then, keep\nthat Control Stick\nsmokin'!\nBuwaa ha ha!") };
static const u8 dialog_text_DIALOG_164[] = { _("Mario! What's up, pal?\nI haven't been on the\nslide lately, so I'm out\nof shape.\nStill, I'm always up for a\ngood race, especially\nagainst an old sleddin'\nbuddy.\nWhaddya say?\nReady...set...\n\n//Go//// Don't Go") };
static const u8 dialog_text_DIALOG_165[] = { _("I take no responsibility\nwhatsoever for those who\nget dizzy and pass out\nfrom running around\nthis post.") };
static const u8 dialog_text_DIALOG_166[] = { _("I'll be back soon.\nI'm out training now,\nso come back later.\n//--Koopa the Quick") };
static const u8 dialog_text_DIALOG_167[] = { _("Princess Toadstool's\ncastle is just ahead.\n\n\nPress [A] to jump, [Z] to\ncrouch, and [B] to punch,\nread a sign, or grab\nsomething.\nPress [B] again to throw\nsomething you're holding.") };
static const u8 dialog_text_DIALOG_168[] = { _("Hey! Knock it off! That's\nthe second time you've\nnailed me. Now you're\nasking for it, linguine\nbreath!") };
static const u8 dialog_text_DIALOG_169[] = { _("Keep out!\nThat means you!\nArrgghh!\n\nAnyone entering this cave\nwithout permission will\nmeet certain disaster.") };
static const struct DialogEntry dialog_entry_DIALOG_000 = { 1, 6, 30, 200, dialog_text_DIALOG_000 };
static const struct DialogEntry dialog_entry_DIALOG_001 = { 1, 4, 95, 200, dialog_text_DIALOG_001 };
static const struct DialogEntry dialog_entry_DIALOG_002 = { 1, 4, 95, 200, dialog_text_DIALOG_002 };
static const struct DialogEntry dialog_entry_DIALOG_003 = { 1, 5, 95, 200, dialog_text_DIALOG_003 };
static const struct DialogEntry dialog_entry_DIALOG_004 = { 1, 3, 95, 200, dialog_text_DIALOG_004 };
static const struct DialogEntry dialog_entry_DIALOG_005 = { 1, 3, 30, 200, dialog_text_DIALOG_005 };
static const struct DialogEntry dialog_entry_DIALOG_006 = { 1, 3, 30, 200, dialog_text_DIALOG_006 };
static const struct DialogEntry dialog_entry_DIALOG_007 = { 1, 5, 30, 200, dialog_text_DIALOG_007 };
static const struct DialogEntry dialog_entry_DIALOG_008 = { 1, 4, 30, 200, dialog_text_DIALOG_008 };
static const struct DialogEntry dialog_entry_DIALOG_009 = { 1, 5, 30, 200, dialog_text_DIALOG_009 };
static const struct DialogEntry dialog_entry_DIALOG_010 = { 1, 4, 30, 200, dialog_text_DIALOG_010 };
static const struct DialogEntry dialog_entry_DIALOG_011 = { 1, 4, 30, 200, dialog_text_DIALOG_011 };
static const struct DialogEntry dialog_entry_DIALOG_012 = { 1, 4, 30, 200, dialog_text_DIALOG_012 };
static const struct DialogEntry dialog_entry_DIALOG_013 = { 1, 5, 30, 200, dialog_text_DIALOG_013 };
static const struct DialogEntry dialog_entry_DIALOG_014 = { 1, 4, 30, 200, dialog_text_DIALOG_014 };
static const struct DialogEntry dialog_entry_DIALOG_015 = { 1, 4, 30, 200, dialog_text_DIALOG_015 };
static const struct DialogEntry dialog_entry_DIALOG_016 = { 1, 3, 30, 200, dialog_text_DIALOG_016 };
static const struct DialogEntry dialog_entry_DIALOG_017 = { 1, 4, 30, 200, dialog_text_DIALOG_017 };
static const struct DialogEntry dialog_entry_DIALOG_018 = { 1, 4, 30, 200, dialog_text_DIALOG_018 };
static const struct DialogEntry dialog_entry_DIALOG_019 = { 1, 2, 30, 200, dialog_text_DIALOG_019 };
static const struct DialogEntry dialog_entry_DIALOG_020 = { 1, 6, 95, 200, dialog_text_DIALOG_020 };
static const struct DialogEntry dialog_entry_DIALOG_021 = { 1, 5, 95, 200, dialog_text_DIALOG_021 };
static const struct DialogEntry dialog_entry_DIALOG_022 = { 1, 2, 95, 200, dialog_text_DIALOG_022 };
static const struct DialogEntry dialog_entry_DIALOG_023 = { 1, 3, 95, 200, dialog_text_DIALOG_023 };
static const struct DialogEntry dialog_entry_DIALOG_024 = { 1, 5, 95, 200, dialog_text_DIALOG_024 };
static const struct DialogEntry dialog_entry_DIALOG_025 = { 1, 4, 95, 200, dialog_text_DIALOG_025 };
static const struct DialogEntry dialog_entry_DIALOG_026 = { 1, 4, 95, 200, dialog_text_DIALOG_026 };
static const struct DialogEntry dialog_entry_DIALOG_027 = { 1, 4, 95, 200, dialog_text_DIALOG_027 };
static const struct DialogEntry dialog_entry_DIALOG_028 = { 1, 4, 95, 200, dialog_text_DIALOG_028 };
static const struct DialogEntry dialog_entry_DIALOG_029 = { 1, 5, 95, 200, dialog_text_DIALOG_029 };
static const struct DialogEntry dialog_entry_DIALOG_030 = { 1, 6, 30, 200, dialog_text_DIALOG_030 };
static const struct DialogEntry dialog_entry_DIALOG_031 = { 1, 5, 30, 200, dialog_text_DIALOG_031 };
static const struct DialogEntry dialog_entry_DIALOG_032 = { 1, 5, 30, 200, dialog_text_DIALOG_032 };
static const struct DialogEntry dialog_entry_DIALOG_033 = { 1, 6, 30, 200, dialog_text_DIALOG_033 };
static const struct DialogEntry dialog_entry_DIALOG_034 = { 1, 6, 30, 200, dialog_text_DIALOG_034 };
static const struct DialogEntry dialog_entry_DIALOG_035 = { 1, 5, 30, 200, dialog_text_DIALOG_035 };
static const struct DialogEntry dialog_entry_DIALOG_036 = { 1, 5, 30, 200, dialog_text_DIALOG_036 };
static const struct DialogEntry dialog_entry_DIALOG_037 = { 1, 2, 30, 200, dialog_text_DIALOG_037 };
static const struct DialogEntry dialog_entry_DIALOG_038 = { 1, 3, 95, 200, dialog_text_DIALOG_038 };
static const struct DialogEntry dialog_entry_DIALOG_039 = { 1, 4, 30, 200, dialog_text_DIALOG_039 };
static const struct DialogEntry dialog_entry_DIALOG_040 = { 1, 3, 30, 200, dialog_text_DIALOG_040 };
static const struct DialogEntry dialog_entry_DIALOG_041 = { 1, 3, 30, 200, dialog_text_DIALOG_041 };
static const struct DialogEntry dialog_entry_DIALOG_042 = { 1, 4, 30, 200, dialog_text_DIALOG_042 };
static const struct DialogEntry dialog_entry_DIALOG_043 = { 1, 5, 30, 200, dialog_text_DIALOG_043 };
static const struct DialogEntry dialog_entry_DIALOG_044 = { 1, 5, 95, 200, dialog_text_DIALOG_044 };
static const struct DialogEntry dialog_entry_DIALOG_045 = { 1, 6, 95, 200, dialog_text_DIALOG_045 };
static const struct DialogEntry dialog_entry_DIALOG_046 = { 1, 5, 30, 200, dialog_text_DIALOG_046 };
static const struct DialogEntry dialog_entry_DIALOG_047 = { 1, 2, 95, 200, dialog_text_DIALOG_047 };
static const struct DialogEntry dialog_entry_DIALOG_048 = { 1, 4, 30, 200, dialog_text_DIALOG_048 };
static const struct DialogEntry dialog_entry_DIALOG_049 = { 1, 5, 30, 200, dialog_text_DIALOG_049 };
static const struct DialogEntry dialog_entry_DIALOG_050 = { 1, 4, 30, 200, dialog_text_DIALOG_050 };
static const struct DialogEntry dialog_entry_DIALOG_051 = { 1, 6, 30, 200, dialog_text_DIALOG_051 };
static const struct DialogEntry dialog_entry_DIALOG_052 = { 1, 5, 30, 200, dialog_text_DIALOG_052 };
static const struct DialogEntry dialog_entry_DIALOG_053 = { 1, 5, 30, 200, dialog_text_DIALOG_053 };
static const struct DialogEntry dialog_entry_DIALOG_054 = { 1, 5, 30, 200, dialog_text_DIALOG_054 };
static const struct DialogEntry dialog_entry_DIALOG_055 = { 1, 4, 30, 200, dialog_text_DIALOG_055 };
static const struct DialogEntry dialog_entry_DIALOG_056 = { 1, 6, 30, 200, dialog_text_DIALOG_056 };
static const struct DialogEntry dialog_entry_DIALOG_057 = { 1, 4, 30, 200, dialog_text_DIALOG_057 };
static const struct DialogEntry dialog_entry_DIALOG_058 = { 1, 4, 30, 200, dialog_text_DIALOG_058 };
static const struct DialogEntry dialog_entry_DIALOG_059 = { 1, 4, 30, 200, dialog_text_DIALOG_059 };
static const struct DialogEntry dialog_entry_DIALOG_060 = { 1, 4, 30, 200, dialog_text_DIALOG_060 };
static const struct DialogEntry dialog_entry_DIALOG_061 = { 1, 4, 30, 200, dialog_text_DIALOG_061 };
static const struct DialogEntry dialog_entry_DIALOG_062 = { 1, 3, 30, 200, dialog_text_DIALOG_062 };
static const struct DialogEntry dialog_entry_DIALOG_063 = { 1, 5, 30, 200, dialog_text_DIALOG_063 };
static const struct DialogEntry dialog_entry_DIALOG_064 = { 1, 5, 30, 200, dialog_text_DIALOG_064 };
static const struct DialogEntry dialog_entry_DIALOG_065 = { 1, 6, 30, 200, dialog_text_DIALOG_065 };
static const struct DialogEntry dialog_entry_DIALOG_066 = { 1, 5, 30, 200, dialog_text_DIALOG_066 };
static const struct DialogEntry dialog_entry_DIALOG_067 = { 1, 5, 30, 200, dialog_text_DIALOG_067 };
static const struct DialogEntry dialog_entry_DIALOG_068 = { 1, 5, 30, 200, dialog_text_DIALOG_068 };
static const struct DialogEntry dialog_entry_DIALOG_069 = { 1, 6, 30, 200, dialog_text_DIALOG_069 };
static const struct DialogEntry dialog_entry_DIALOG_070 = { 1, 5, 30, 200, dialog_text_DIALOG_070 };
static const struct DialogEntry dialog_entry_DIALOG_071 = { 1, 3, 30, 200, dialog_text_DIALOG_071 };
static const struct DialogEntry dialog_entry_DIALOG_072 = { 1, 5, 30, 200, dialog_text_DIALOG_072 };
static const struct DialogEntry dialog_entry_DIALOG_073 = { 1, 4, 95, 200, dialog_text_DIALOG_073 };
static const struct DialogEntry dialog_entry_DIALOG_074 = { 1, 5, 30, 200, dialog_text_DIALOG_074 };
static const struct DialogEntry dialog_entry_DIALOG_075 = { 1, 5, 30, 200, dialog_text_DIALOG_075 };
static const struct DialogEntry dialog_entry_DIALOG_076 = { 1, 6, 30, 200, dialog_text_DIALOG_076 };
static const struct DialogEntry dialog_entry_DIALOG_077 = { 1, 2, 130, 200, dialog_text_DIALOG_077 };
static const struct DialogEntry dialog_entry_DIALOG_078 = { 1, 5, 30, 200, dialog_text_DIALOG_078 };
static const struct DialogEntry dialog_entry_DIALOG_079 = { 1, 4, 30, 200, dialog_text_DIALOG_079 };
static const struct DialogEntry dialog_entry_DIALOG_080 = { 1, 1, 30, 200, dialog_text_DIALOG_080 };
static const struct DialogEntry dialog_entry_DIALOG_081 = { 1, 4, 30, 200, dialog_text_DIALOG_081 };
static const struct DialogEntry dialog_entry_DIALOG_082 = { 1, 4, 30, 200, dialog_text_DIALOG_082 };
static const struct DialogEntry dialog_entry_DIALOG_083 = { 1, 6, 30, 200, dialog_text_DIALOG_083 };
static const struct DialogEntry dialog_entry_DIALOG_084 = { 1, 3, 30, 200, dialog_text_DIALOG_084 };
static const struct DialogEntry dialog_entry_DIALOG_085 = { 1, 5, 30, 200, dialog_text_DIALOG_085 };
static const struct DialogEntry dialog_entry_DIALOG_086 = { 1, 3, 30, 200, dialog_text_DIALOG_086 };
static const struct DialogEntry dialog_entry_DIALOG_087 = { 1, 4, 30, 200, dialog_text_DIALOG_087 };
static const struct DialogEntry dialog_entry_DIALOG_088 = { 1, 5, 30, 200, dialog_text_DIALOG_088 };
static const struct DialogEntry dialog_entry_DIALOG_089 = { 1, 5, 95, 200, dialog_text_DIALOG_089 };
static const struct DialogEntry dialog_entry_DIALOG_090 = { 1, 6, 30, 200, dialog_text_DIALOG_090 };
static const struct DialogEntry dialog_entry_DIALOG_091 = { 2, 2, 30, 200, dialog_text_DIALOG_091 };
static const struct DialogEntry dialog_entry_DIALOG_092 = { 1, 5, 30, 200, dialog_text_DIALOG_092 };
static const struct DialogEntry dialog_entry_DIALOG_093 = { 1, 5, 30, 200, dialog_text_DIALOG_093 };
static const struct DialogEntry dialog_entry_DIALOG_094 = { 1, 4, 30, 200, dialog_text_DIALOG_094 };
static const struct DialogEntry dialog_entry_DIALOG_095 = { 1, 4, 30, 200, dialog_text_DIALOG_095 };
static const struct DialogEntry dialog_entry_DIALOG_096 = { 1, 4, 30, 200, dialog_text_DIALOG_096 };
static const struct DialogEntry dialog_entry_DIALOG_097 = { 1, 5, 30, 200, dialog_text_DIALOG_097 };
static const struct DialogEntry dialog_entry_DIALOG_098 = { 1, 2, 95, 200, dialog_text_DIALOG_098 };
static const struct DialogEntry dialog_entry_DIALOG_099 = { 1, 5, 95, 200, dialog_text_DIALOG_099 };
static const struct DialogEntry dialog_entry_DIALOG_100 = { 1, 3, 95, 200, dialog_text_DIALOG_100 };
static const struct DialogEntry dialog_entry_DIALOG_101 = { 1, 3, 95, 200, dialog_text_DIALOG_101 };
static const struct DialogEntry dialog_entry_DIALOG_102 = { 1, 5, 30, 200, dialog_text_DIALOG_102 };
static const struct DialogEntry dialog_entry_DIALOG_103 = { 1, 4, 95, 200, dialog_text_DIALOG_103 };
static const struct DialogEntry dialog_entry_DIALOG_104 = { 1, 5, 30, 200, dialog_text_DIALOG_104 };
static const struct DialogEntry dialog_entry_DIALOG_105 = { 1, 3, 95, 200, dialog_text_DIALOG_105 };
static const struct DialogEntry dialog_entry_DIALOG_106 = { 1, 2, 95, 200, dialog_text_DIALOG_106 };
static const struct DialogEntry dialog_entry_DIALOG_107 = { 1, 3, 95, 200, dialog_text_DIALOG_107 };
static const struct DialogEntry dialog_entry_DIALOG_108 = { 1, 2, 95, 200, dialog_text_DIALOG_108 };
static const struct DialogEntry dialog_entry_DIALOG_109 = { 1, 4, 95, 200, dialog_text_DIALOG_109 };
static const struct DialogEntry dialog_entry_DIALOG_110 = { 1, 5, 95, 200, dialog_text_DIALOG_110 };
static const struct DialogEntry dialog_entry_DIALOG_111 = { 1, 4, 95, 200, dialog_text_DIALOG_111 };
static const struct DialogEntry dialog_entry_DIALOG_112 = { 1, 4, 30, 200, dialog_text_DIALOG_112 };
static const struct DialogEntry dialog_entry_DIALOG_113 = { 1, 6, 30, 200, dialog_text_DIALOG_113 };
static const struct DialogEntry dialog_entry_DIALOG_114 = { 1, 5, 95, 200, dialog_text_DIALOG_114 };
static const struct DialogEntry dialog_entry_DIALOG_115 = { 1, 5, 95, 200, dialog_text_DIALOG_115 };
static const struct DialogEntry dialog_entry_DIALOG_116 = { 1, 5, 95, 200, dialog_text_DIALOG_116 };
static const struct DialogEntry dialog_entry_DIALOG_117 = { 1, 1, 95, 200, dialog_text_DIALOG_117 };
static const struct DialogEntry dialog_entry_DIALOG_118 = { 1, 6, 95, 200, dialog_text_DIALOG_118 };
static const struct DialogEntry dialog_entry_DIALOG_119 = { 1, 6, 30, 200, dialog_text_DIALOG_119 };
static const struct DialogEntry dialog_entry_DIALOG_120 = { 1, 4, 30, 200, dialog_text_DIALOG_120 };
static const struct DialogEntry dialog_entry_DIALOG_121 = { 1, 5, 30, 200, dialog_text_DIALOG_121 };
static const struct DialogEntry dialog_entry_DIALOG_122 = { 1, 4, 30, 200, dialog_text_DIALOG_122 };
static const struct DialogEntry dialog_entry_DIALOG_123 = { 1, 4, 30, 200, dialog_text_DIALOG_123 };
static const struct DialogEntry dialog_entry_DIALOG_124 = { 1, 4, 30, 200, dialog_text_DIALOG_124 };
static const struct DialogEntry dialog_entry_DIALOG_125 = { 1, 3, 30, 200, dialog_text_DIALOG_125 };
static const struct DialogEntry dialog_entry_DIALOG_126 = { 2, 3, 30, 200, dialog_text_DIALOG_126 };
static const struct DialogEntry dialog_entry_DIALOG_127 = { 3, 4, 30, 200, dialog_text_DIALOG_127 };
static const struct DialogEntry dialog_entry_DIALOG_128 = { 1, 4, 95, 200, dialog_text_DIALOG_128 };
static const struct DialogEntry dialog_entry_DIALOG_129 = { 1, 5, 30, 200, dialog_text_DIALOG_129 };
static const struct DialogEntry dialog_entry_DIALOG_130 = { 1, 5, 30, 200, dialog_text_DIALOG_130 };
static const struct DialogEntry dialog_entry_DIALOG_131 = { 1, 5, 30, 200, dialog_text_DIALOG_131 };
static const struct DialogEntry dialog_entry_DIALOG_132 = { 1, 4, 30, 200, dialog_text_DIALOG_132 };
static const struct DialogEntry dialog_entry_DIALOG_133 = { 1, 6, 30, 200, dialog_text_DIALOG_133 };
static const struct DialogEntry dialog_entry_DIALOG_134 = { 1, 5, 30, 200, dialog_text_DIALOG_134 };
static const struct DialogEntry dialog_entry_DIALOG_135 = { 1, 5, 30, 200, dialog_text_DIALOG_135 };
static const struct DialogEntry dialog_entry_DIALOG_136 = { 1, 6, 30, 200, dialog_text_DIALOG_136 };
static const struct DialogEntry dialog_entry_DIALOG_137 = { 1, 6, 30, 200, dialog_text_DIALOG_137 };
static const struct DialogEntry dialog_entry_DIALOG_138 = { 1, 3, 30, 200, dialog_text_DIALOG_138 };
static const struct DialogEntry dialog_entry_DIALOG_139 = { 1, 6, 30, 200, dialog_text_DIALOG_139 };
static const struct DialogEntry dialog_entry_DIALOG_140 = { 1, 6, 30, 200, dialog_text_DIALOG_140 };
static const struct DialogEntry dialog_entry_DIALOG_141 = { 1, 5, 130, 200, dialog_text_DIALOG_141 };
static const struct DialogEntry dialog_entry_DIALOG_142 = { 1, 5, 130, 200, dialog_text_DIALOG_142 };
static const struct DialogEntry dialog_entry_DIALOG_143 = { 1, 6, 130, 200, dialog_text_DIALOG_143 };
static const struct DialogEntry dialog_entry_DIALOG_144 = { 1, 6, 130, 200, dialog_text_DIALOG_144 };
static const struct DialogEntry dialog_entry_DIALOG_145 = { 1, 6, 130, 200, dialog_text_DIALOG_145 };
static const struct DialogEntry dialog_entry_DIALOG_146 = { 1, 6, 130, 200, dialog_text_DIALOG_146 };
static const struct DialogEntry dialog_entry_DIALOG_147 = { 1, 5, 30, 200, dialog_text_DIALOG_147 };
static const struct DialogEntry dialog_entry_DIALOG_148 = { 1, 6, 30, 200, dialog_text_DIALOG_148 };
static const struct DialogEntry dialog_entry_DIALOG_149 = { 1, 3, 30, 200, dialog_text_DIALOG_149 };
static const struct DialogEntry dialog_entry_DIALOG_150 = { 1, 5, 30, 200, dialog_text_DIALOG_150 };
static const struct DialogEntry dialog_entry_DIALOG_151 = { 1, 4, 30, 200, dialog_text_DIALOG_151 };
static const struct DialogEntry dialog_entry_DIALOG_152 = { 1, 3, 30, 200, dialog_text_DIALOG_152 };
static const struct DialogEntry dialog_entry_DIALOG_153 = { 1, 4, 30, 200, dialog_text_DIALOG_153 };
static const struct DialogEntry dialog_entry_DIALOG_154 = { 1, 5, 30, 200, dialog_text_DIALOG_154 };
static const struct DialogEntry dialog_entry_DIALOG_155 = { 1, 6, 30, 200, dialog_text_DIALOG_155 };
static const struct DialogEntry dialog_entry_DIALOG_156 = { 1, 5, 30, 200, dialog_text_DIALOG_156 };
static const struct DialogEntry dialog_entry_DIALOG_157 = { 1, 5, 30, 200, dialog_text_DIALOG_157 };
static const struct DialogEntry dialog_entry_DIALOG_158 = { 1, 6, 30, 200, dialog_text_DIALOG_158 };
static const struct DialogEntry dialog_entry_DIALOG_159 = { 1, 6, 30, 200, dialog_text_DIALOG_159 };
static const struct DialogEntry dialog_entry_DIALOG_160 = { 1, 4, 30, 200, dialog_text_DIALOG_160 };
static const struct DialogEntry dialog_entry_DIALOG_161 = { 1, 4, 30, 200, dialog_text_DIALOG_161 };
static const struct DialogEntry dialog_entry_DIALOG_162 = { 1, 4, 30, 200, dialog_text_DIALOG_162 };
static const struct DialogEntry dialog_entry_DIALOG_163 = { 1, 5, 30, 200, dialog_text_DIALOG_163 };
static const struct DialogEntry dialog_entry_DIALOG_164 = { 1, 4, 30, 200, dialog_text_DIALOG_164 };
static const struct DialogEntry dialog_entry_DIALOG_165 = { 1, 5, 30, 200, dialog_text_DIALOG_165 };
static const struct DialogEntry dialog_entry_DIALOG_166 = { 1, 4, 30, 200, dialog_text_DIALOG_166 };
static const struct DialogEntry dialog_entry_DIALOG_167 = { 1, 4, 30, 200, dialog_text_DIALOG_167 };
static const struct DialogEntry dialog_entry_DIALOG_168 = { 1, 5, 30, 200, dialog_text_DIALOG_168 };
static const struct DialogEntry dialog_entry_DIALOG_169 = { 1, 4, 30, 200, dialog_text_DIALOG_169 };
const struct DialogEntry *const seg2_dialog_table[] = {
&dialog_entry_DIALOG_000,
&dialog_entry_DIALOG_001,
&dialog_entry_DIALOG_002,
&dialog_entry_DIALOG_003,
&dialog_entry_DIALOG_004,
&dialog_entry_DIALOG_005,
&dialog_entry_DIALOG_006,
&dialog_entry_DIALOG_007,
&dialog_entry_DIALOG_008,
&dialog_entry_DIALOG_009,
&dialog_entry_DIALOG_010,
&dialog_entry_DIALOG_011,
&dialog_entry_DIALOG_012,
&dialog_entry_DIALOG_013,
&dialog_entry_DIALOG_014,
&dialog_entry_DIALOG_015,
&dialog_entry_DIALOG_016,
&dialog_entry_DIALOG_017,
&dialog_entry_DIALOG_018,
&dialog_entry_DIALOG_019,
&dialog_entry_DIALOG_020,
&dialog_entry_DIALOG_021,
&dialog_entry_DIALOG_022,
&dialog_entry_DIALOG_023,
&dialog_entry_DIALOG_024,
&dialog_entry_DIALOG_025,
&dialog_entry_DIALOG_026,
&dialog_entry_DIALOG_027,
&dialog_entry_DIALOG_028,
&dialog_entry_DIALOG_029,
&dialog_entry_DIALOG_030,
&dialog_entry_DIALOG_031,
&dialog_entry_DIALOG_032,
&dialog_entry_DIALOG_033,
&dialog_entry_DIALOG_034,
&dialog_entry_DIALOG_035,
&dialog_entry_DIALOG_036,
&dialog_entry_DIALOG_037,
&dialog_entry_DIALOG_038,
&dialog_entry_DIALOG_039,
&dialog_entry_DIALOG_040,
&dialog_entry_DIALOG_041,
&dialog_entry_DIALOG_042,
&dialog_entry_DIALOG_043,
&dialog_entry_DIALOG_044,
&dialog_entry_DIALOG_045,
&dialog_entry_DIALOG_046,
&dialog_entry_DIALOG_047,
&dialog_entry_DIALOG_048,
&dialog_entry_DIALOG_049,
&dialog_entry_DIALOG_050,
&dialog_entry_DIALOG_051,
&dialog_entry_DIALOG_052,
&dialog_entry_DIALOG_053,
&dialog_entry_DIALOG_054,
&dialog_entry_DIALOG_055,
&dialog_entry_DIALOG_056,
&dialog_entry_DIALOG_057,
&dialog_entry_DIALOG_058,
&dialog_entry_DIALOG_059,
&dialog_entry_DIALOG_060,
&dialog_entry_DIALOG_061,
&dialog_entry_DIALOG_062,
&dialog_entry_DIALOG_063,
&dialog_entry_DIALOG_064,
&dialog_entry_DIALOG_065,
&dialog_entry_DIALOG_066,
&dialog_entry_DIALOG_067,
&dialog_entry_DIALOG_068,
&dialog_entry_DIALOG_069,
&dialog_entry_DIALOG_070,
&dialog_entry_DIALOG_071,
&dialog_entry_DIALOG_072,
&dialog_entry_DIALOG_073,
&dialog_entry_DIALOG_074,
&dialog_entry_DIALOG_075,
&dialog_entry_DIALOG_076,
&dialog_entry_DIALOG_077,
&dialog_entry_DIALOG_078,
&dialog_entry_DIALOG_079,
&dialog_entry_DIALOG_080,
&dialog_entry_DIALOG_081,
&dialog_entry_DIALOG_082,
&dialog_entry_DIALOG_083,
&dialog_entry_DIALOG_084,
&dialog_entry_DIALOG_085,
&dialog_entry_DIALOG_086,
&dialog_entry_DIALOG_087,
&dialog_entry_DIALOG_088,
&dialog_entry_DIALOG_089,
&dialog_entry_DIALOG_090,
&dialog_entry_DIALOG_091,
&dialog_entry_DIALOG_092,
&dialog_entry_DIALOG_093,
&dialog_entry_DIALOG_094,
&dialog_entry_DIALOG_095,
&dialog_entry_DIALOG_096,
&dialog_entry_DIALOG_097,
&dialog_entry_DIALOG_098,
&dialog_entry_DIALOG_099,
&dialog_entry_DIALOG_100,
&dialog_entry_DIALOG_101,
&dialog_entry_DIALOG_102,
&dialog_entry_DIALOG_103,
&dialog_entry_DIALOG_104,
&dialog_entry_DIALOG_105,
&dialog_entry_DIALOG_106,
&dialog_entry_DIALOG_107,
&dialog_entry_DIALOG_108,
&dialog_entry_DIALOG_109,
&dialog_entry_DIALOG_110,
&dialog_entry_DIALOG_111,
&dialog_entry_DIALOG_112,
&dialog_entry_DIALOG_113,
&dialog_entry_DIALOG_114,
&dialog_entry_DIALOG_115,
&dialog_entry_DIALOG_116,
&dialog_entry_DIALOG_117,
&dialog_entry_DIALOG_118,
&dialog_entry_DIALOG_119,
&dialog_entry_DIALOG_120,
&dialog_entry_DIALOG_121,
&dialog_entry_DIALOG_122,
&dialog_entry_DIALOG_123,
&dialog_entry_DIALOG_124,
&dialog_entry_DIALOG_125,
&dialog_entry_DIALOG_126,
&dialog_entry_DIALOG_127,
&dialog_entry_DIALOG_128,
&dialog_entry_DIALOG_129,
&dialog_entry_DIALOG_130,
&dialog_entry_DIALOG_131,
&dialog_entry_DIALOG_132,
&dialog_entry_DIALOG_133,
&dialog_entry_DIALOG_134,
&dialog_entry_DIALOG_135,
&dialog_entry_DIALOG_136,
&dialog_entry_DIALOG_137,
&dialog_entry_DIALOG_138,
&dialog_entry_DIALOG_139,
&dialog_entry_DIALOG_140,
&dialog_entry_DIALOG_141,
&dialog_entry_DIALOG_142,
&dialog_entry_DIALOG_143,
&dialog_entry_DIALOG_144,
&dialog_entry_DIALOG_145,
&dialog_entry_DIALOG_146,
&dialog_entry_DIALOG_147,
&dialog_entry_DIALOG_148,
&dialog_entry_DIALOG_149,
&dialog_entry_DIALOG_150,
&dialog_entry_DIALOG_151,
&dialog_entry_DIALOG_152,
&dialog_entry_DIALOG_153,
&dialog_entry_DIALOG_154,
&dialog_entry_DIALOG_155,
&dialog_entry_DIALOG_156,
&dialog_entry_DIALOG_157,
&dialog_entry_DIALOG_158,
&dialog_entry_DIALOG_159,
&dialog_entry_DIALOG_160,
&dialog_entry_DIALOG_161,
&dialog_entry_DIALOG_162,
&dialog_entry_DIALOG_163,
&dialog_entry_DIALOG_164,
&dialog_entry_DIALOG_165,
&dialog_entry_DIALOG_166,
&dialog_entry_DIALOG_167,
&dialog_entry_DIALOG_168,
&dialog_entry_DIALOG_169,
    NULL
};
static const u8 GLUE2(seg2_course_name_table, _COURSE_BOB)[] = { _(" 1 BOB-OMB BATTLEFIELD") };
static const u8 GLUE2(seg2_course_name_table, _COURSE_WF)[] = { _(" 2 WHOMP'S FORTRESS") };
static const u8 GLUE2(seg2_course_name_table, _COURSE_JRB)[] = { _(" 3 JOLLY ROGER BAY") };
static const u8 GLUE2(seg2_course_name_table, _COURSE_CCM)[] = { _(" 4 COOL, COOL MOUNTAIN") };
static const u8 GLUE2(seg2_course_name_table, _COURSE_BBH)[] = { _(" 5 BIG BOO'S HAUNT") };
static const u8 GLUE2(seg2_course_name_table, _COURSE_HMC)[] = { _(" 6 HAZY MAZE CAVE") };
static const u8 GLUE2(seg2_course_name_table, _COURSE_LLL)[] = { _(" 7 LETHAL LAVA LAND") };
static const u8 GLUE2(seg2_course_name_table, _COURSE_SSL)[] = { _(" 8 SHIFTING SAND LAND") };
static const u8 GLUE2(seg2_course_name_table, _COURSE_DDD)[] = { _(" 9 DIRE, DIRE DOCKS") };
static const u8 GLUE2(seg2_course_name_table, _COURSE_SL)[] = { _("10 SNOWMAN'S LAND") };
static const u8 GLUE2(seg2_course_name_table, _COURSE_WDW)[] = { _("11 WET-DRY WORLD") };
static const u8 GLUE2(seg2_course_name_table, _COURSE_TTM)[] = { _("12 TALL, TALL MOUNTAIN") };
static const u8 GLUE2(seg2_course_name_table, _COURSE_THI)[] = { _("13 TINY-HUGE ISLAND") };
static const u8 GLUE2(seg2_course_name_table, _COURSE_TTC)[] = { _("14 TICK TOCK CLOCK") };
static const u8 GLUE2(seg2_course_name_table, _COURSE_RR)[] = { _("15 RAINBOW RIDE") };
static const u8 GLUE2(seg2_course_name_table, _COURSE_BITDW)[] = { _("   BOWSER IN THE DARK WORLD") };
static const u8 GLUE2(seg2_course_name_table, _COURSE_BITFS)[] = { _("   BOWSER IN THE FIRE SEA") };
static const u8 GLUE2(seg2_course_name_table, _COURSE_BITS)[] = { _("   BOWSER IN THE SKY") };
static const u8 GLUE2(seg2_course_name_table, _COURSE_PSS)[] = { _("   THE PRINCESS'S SECRET SLIDE") };
static const u8 GLUE2(seg2_course_name_table, _COURSE_COTMC)[] = { _("   CAVERN OF THE METAL CAP") };
static const u8 GLUE2(seg2_course_name_table, _COURSE_TOTWC)[] = { _("   TOWER OF THE WING CAP") };
static const u8 GLUE2(seg2_course_name_table, _COURSE_VCUTM)[] = { _("   VANISH CAP UNDER THE MOAT") };
static const u8 GLUE2(seg2_course_name_table, _COURSE_WMOTR)[] = { _("   WING MARIO OVER THE RAINBOW") };
static const u8 GLUE2(seg2_course_name_table, _COURSE_SA)[] = { _("   THE SECRET AQUARIUM") };
static const u8 GLUE2(seg2_course_name_table, _COURSE_CAKE_END)[] = { _("") };
static const u8 GLUE2(seg2_course_name_table, _castle_secret_stars)[] = { _("   CASTLE SECRET STARS") };







const u8 *const seg2_course_name_table[] = {
GLUE2(seg2_course_name_table, _COURSE_BOB),
GLUE2(seg2_course_name_table, _COURSE_WF),
GLUE2(seg2_course_name_table, _COURSE_JRB),
GLUE2(seg2_course_name_table, _COURSE_CCM),
GLUE2(seg2_course_name_table, _COURSE_BBH),
GLUE2(seg2_course_name_table, _COURSE_HMC),
GLUE2(seg2_course_name_table, _COURSE_LLL),
GLUE2(seg2_course_name_table, _COURSE_SSL),
GLUE2(seg2_course_name_table, _COURSE_DDD),
GLUE2(seg2_course_name_table, _COURSE_SL),
GLUE2(seg2_course_name_table, _COURSE_WDW),
GLUE2(seg2_course_name_table, _COURSE_TTM),
GLUE2(seg2_course_name_table, _COURSE_THI),
GLUE2(seg2_course_name_table, _COURSE_TTC),
GLUE2(seg2_course_name_table, _COURSE_RR),
GLUE2(seg2_course_name_table, _COURSE_BITDW),
GLUE2(seg2_course_name_table, _COURSE_BITFS),
GLUE2(seg2_course_name_table, _COURSE_BITS),
GLUE2(seg2_course_name_table, _COURSE_PSS),
GLUE2(seg2_course_name_table, _COURSE_COTMC),
GLUE2(seg2_course_name_table, _COURSE_TOTWC),
GLUE2(seg2_course_name_table, _COURSE_VCUTM),
GLUE2(seg2_course_name_table, _COURSE_WMOTR),
GLUE2(seg2_course_name_table, _COURSE_SA),
GLUE2(seg2_course_name_table, _COURSE_CAKE_END),
GLUE2(seg2_course_name_table, _castle_secret_stars),







    NULL
};
static const u8 act_name_COURSE_BOB_1[] = { _("BIG BOB-OMB ON THE SUMMIT") }; static const u8 act_name_COURSE_BOB_2[] = { _("FOOTRACE WITH KOOPA THE QUICK") }; static const u8 act_name_COURSE_BOB_3[] = { _("SHOOT TO THE ISLAND IN THE SKY") }; static const u8 act_name_COURSE_BOB_4[] = { _("FIND THE 8 RED COINS") }; static const u8 act_name_COURSE_BOB_5[] = { _("MARIO WINGS TO THE SKY") }; static const u8 act_name_COURSE_BOB_6[] = { _("BEHIND CHAIN CHOMP'S GATE") };
static const u8 act_name_COURSE_WF_1[] = { _("CHIP OFF WHOMP'S BLOCK") }; static const u8 act_name_COURSE_WF_2[] = { _("TO THE TOP OF THE FORTRESS") }; static const u8 act_name_COURSE_WF_3[] = { _("SHOOT INTO THE WILD BLUE") }; static const u8 act_name_COURSE_WF_4[] = { _("RED COINS ON THE FLOATING ISLE") }; static const u8 act_name_COURSE_WF_5[] = { _("FALL ONTO THE CAGED ISLAND") }; static const u8 act_name_COURSE_WF_6[] = { _("BLAST AWAY THE WALL") };
static const u8 act_name_COURSE_JRB_1[] = { _("PLUNDER IN THE SUNKEN SHIP") }; static const u8 act_name_COURSE_JRB_2[] = { _("CAN THE EEL COME OUT TO PLAY?") }; static const u8 act_name_COURSE_JRB_3[] = { _("TREASURE OF THE OCEAN CAVE") }; static const u8 act_name_COURSE_JRB_4[] = { _("RED COINS ON THE SHIP AFLOAT") }; static const u8 act_name_COURSE_JRB_5[] = { _("BLAST TO THE STONE PILLAR") }; static const u8 act_name_COURSE_JRB_6[] = { _("THROUGH THE JET STREAM") };
static const u8 act_name_COURSE_CCM_1[] = { _("SLIP SLIDIN' AWAY") }; static const u8 act_name_COURSE_CCM_2[] = { _("LI'L PENGUIN LOST") }; static const u8 act_name_COURSE_CCM_3[] = { _("BIG PENGUIN RACE") }; static const u8 act_name_COURSE_CCM_4[] = { _("FROSTY SLIDE FOR 8 RED COINS") }; static const u8 act_name_COURSE_CCM_5[] = { _("SNOWMAN'S LOST HIS HEAD") }; static const u8 act_name_COURSE_CCM_6[] = { _("WALL KICKS WILL WORK") };
static const u8 act_name_COURSE_BBH_1[] = { _("GO ON A GHOST HUNT") }; static const u8 act_name_COURSE_BBH_2[] = { _("RIDE BIG BOO'S MERRY-GO-ROUND") }; static const u8 act_name_COURSE_BBH_3[] = { _("SECRET OF THE HAUNTED BOOKS") }; static const u8 act_name_COURSE_BBH_4[] = { _("SEEK THE 8 RED COINS") }; static const u8 act_name_COURSE_BBH_5[] = { _("BIG BOO'S BALCONY") }; static const u8 act_name_COURSE_BBH_6[] = { _("EYE TO EYE IN THE SECRET ROOM") };
static const u8 act_name_COURSE_HMC_1[] = { _("SWIMMING BEAST IN THE CAVERN") }; static const u8 act_name_COURSE_HMC_2[] = { _("ELEVATE FOR 8 RED COINS") }; static const u8 act_name_COURSE_HMC_3[] = { _("METAL-HEAD MARIO CAN MOVE!") }; static const u8 act_name_COURSE_HMC_4[] = { _("NAVIGATING THE TOXIC MAZE") }; static const u8 act_name_COURSE_HMC_5[] = { _("A-MAZE-ING EMERGENCY EXIT") }; static const u8 act_name_COURSE_HMC_6[] = { _("WATCH FOR ROLLING ROCKS") };
static const u8 act_name_COURSE_LLL_1[] = { _("BOIL THE BIG BULLY") }; static const u8 act_name_COURSE_LLL_2[] = { _("BULLY THE BULLIES") }; static const u8 act_name_COURSE_LLL_3[] = { _("8-COIN PUZZLE WITH 15 PIECES") }; static const u8 act_name_COURSE_LLL_4[] = { _("RED-HOT LOG ROLLING") }; static const u8 act_name_COURSE_LLL_5[] = { _("HOT-FOOT-IT INTO THE VOLCANO") }; static const u8 act_name_COURSE_LLL_6[] = { _("ELEVATOR TOUR IN THE VOLCANO") };
static const u8 act_name_COURSE_SSL_1[] = { _("IN THE TALONS OF THE BIG BIRD") }; static const u8 act_name_COURSE_SSL_2[] = { _("SHINING ATOP THE PYRAMID") }; static const u8 act_name_COURSE_SSL_3[] = { _("INSIDE THE ANCIENT PYRAMID") }; static const u8 act_name_COURSE_SSL_4[] = { _("STAND TALL ON THE FOUR PILLARS") }; static const u8 act_name_COURSE_SSL_5[] = { _("FREE FLYING FOR 8 RED COINS") }; static const u8 act_name_COURSE_SSL_6[] = { _("PYRAMID PUZZLE") };
static const u8 act_name_COURSE_DDD_1[] = { _("BOARD BOWSER'S SUB") }; static const u8 act_name_COURSE_DDD_2[] = { _("CHESTS IN THE CURRENT") }; static const u8 act_name_COURSE_DDD_3[] = { _("POLE-JUMPING FOR RED COINS") }; static const u8 act_name_COURSE_DDD_4[] = { _("THROUGH THE JET STREAM") }; static const u8 act_name_COURSE_DDD_5[] = { _("THE MANTA RAY'S REWARD") }; static const u8 act_name_COURSE_DDD_6[] = { _("COLLECT THE CAPS...") };
static const u8 act_name_COURSE_SL_1[] = { _("SNOWMAN'S BIG HEAD") }; static const u8 act_name_COURSE_SL_2[] = { _("CHILL WITH THE BULLY") }; static const u8 act_name_COURSE_SL_3[] = { _("IN THE DEEP FREEZE") }; static const u8 act_name_COURSE_SL_4[] = { _("WHIRL FROM THE FREEZING POND") }; static const u8 act_name_COURSE_SL_5[] = { _("SHELL SHREDDIN' FOR RED COINS") }; static const u8 act_name_COURSE_SL_6[] = { _("INTO THE IGLOO") };
static const u8 act_name_COURSE_WDW_1[] = { _("SHOCKING ARROW LIFTS!") }; static const u8 act_name_COURSE_WDW_2[] = { _("TOP O' THE TOWN") }; static const u8 act_name_COURSE_WDW_3[] = { _("SECRETS IN THE SHALLOWS & SKY") }; static const u8 act_name_COURSE_WDW_4[] = { _("EXPRESS ELEVATOR--HURRY UP!") }; static const u8 act_name_COURSE_WDW_5[] = { _("GO TO TOWN FOR RED COINS") }; static const u8 act_name_COURSE_WDW_6[] = { _("QUICK RACE THROUGH DOWNTOWN!") };
static const u8 act_name_COURSE_TTM_1[] = { _("SCALE THE MOUNTAIN") }; static const u8 act_name_COURSE_TTM_2[] = { _("MYSTERY OF THE MONKEY CAGE") }; static const u8 act_name_COURSE_TTM_3[] = { _("SCARY 'SHROOMS, RED COINS") }; static const u8 act_name_COURSE_TTM_4[] = { _("MYSTERIOUS MOUNTAINSIDE") }; static const u8 act_name_COURSE_TTM_5[] = { _("BREATHTAKING VIEW FROM BRIDGE") }; static const u8 act_name_COURSE_TTM_6[] = { _("BLAST TO THE LONELY MUSHROOM") };
static const u8 act_name_COURSE_THI_1[] = { _("PLUCK THE PIRANHA FLOWER") }; static const u8 act_name_COURSE_THI_2[] = { _("THE TIP TOP OF THE HUGE ISLAND") }; static const u8 act_name_COURSE_THI_3[] = { _("REMATCH WITH KOOPA THE QUICK") }; static const u8 act_name_COURSE_THI_4[] = { _("FIVE ITTY BITTY SECRETS") }; static const u8 act_name_COURSE_THI_5[] = { _("WIGGLER'S RED COINS") }; static const u8 act_name_COURSE_THI_6[] = { _("MAKE WIGGLER SQUIRM") };
static const u8 act_name_COURSE_TTC_1[] = { _("ROLL INTO THE CAGE") }; static const u8 act_name_COURSE_TTC_2[] = { _("THE PIT AND THE PENDULUMS") }; static const u8 act_name_COURSE_TTC_3[] = { _("GET A HAND") }; static const u8 act_name_COURSE_TTC_4[] = { _("STOMP ON THE THWOMP") }; static const u8 act_name_COURSE_TTC_5[] = { _("TIMED JUMPS ON MOVING BARS") }; static const u8 act_name_COURSE_TTC_6[] = { _("STOP TIME FOR RED COINS") };
static const u8 act_name_COURSE_RR_1[] = { _("CRUISER CROSSING THE RAINBOW") }; static const u8 act_name_COURSE_RR_2[] = { _("THE BIG HOUSE IN THE SKY") }; static const u8 act_name_COURSE_RR_3[] = { _("COINS AMASSED IN A MAZE") }; static const u8 act_name_COURSE_RR_4[] = { _("SWINGIN' IN THE BREEZE") }; static const u8 act_name_COURSE_RR_5[] = { _("TRICKY TRIANGLES!") }; static const u8 act_name_COURSE_RR_6[] = { _("SOMEWHERE OVER THE RAINBOW") };











static const u8 extra_text_0[] = { _("ONE OF THE CASTLE'S SECRET STARS!") };
static const u8 extra_text_1[] = { _("") };
static const u8 extra_text_2[] = { _("") };
static const u8 extra_text_3[] = { _("") };
static const u8 extra_text_4[] = { _("") };
static const u8 extra_text_5[] = { _("") };
static const u8 extra_text_6[] = { _("") };
const u8 *const seg2_act_name_table[] = {
act_name_COURSE_BOB_1, act_name_COURSE_BOB_2, act_name_COURSE_BOB_3, act_name_COURSE_BOB_4, act_name_COURSE_BOB_5, act_name_COURSE_BOB_6,
act_name_COURSE_WF_1, act_name_COURSE_WF_2, act_name_COURSE_WF_3, act_name_COURSE_WF_4, act_name_COURSE_WF_5, act_name_COURSE_WF_6,
act_name_COURSE_JRB_1, act_name_COURSE_JRB_2, act_name_COURSE_JRB_3, act_name_COURSE_JRB_4, act_name_COURSE_JRB_5, act_name_COURSE_JRB_6,
act_name_COURSE_CCM_1, act_name_COURSE_CCM_2, act_name_COURSE_CCM_3, act_name_COURSE_CCM_4, act_name_COURSE_CCM_5, act_name_COURSE_CCM_6,
act_name_COURSE_BBH_1, act_name_COURSE_BBH_2, act_name_COURSE_BBH_3, act_name_COURSE_BBH_4, act_name_COURSE_BBH_5, act_name_COURSE_BBH_6,
act_name_COURSE_HMC_1, act_name_COURSE_HMC_2, act_name_COURSE_HMC_3, act_name_COURSE_HMC_4, act_name_COURSE_HMC_5, act_name_COURSE_HMC_6,
act_name_COURSE_LLL_1, act_name_COURSE_LLL_2, act_name_COURSE_LLL_3, act_name_COURSE_LLL_4, act_name_COURSE_LLL_5, act_name_COURSE_LLL_6,
act_name_COURSE_SSL_1, act_name_COURSE_SSL_2, act_name_COURSE_SSL_3, act_name_COURSE_SSL_4, act_name_COURSE_SSL_5, act_name_COURSE_SSL_6,
act_name_COURSE_DDD_1, act_name_COURSE_DDD_2, act_name_COURSE_DDD_3, act_name_COURSE_DDD_4, act_name_COURSE_DDD_5, act_name_COURSE_DDD_6,
act_name_COURSE_SL_1, act_name_COURSE_SL_2, act_name_COURSE_SL_3, act_name_COURSE_SL_4, act_name_COURSE_SL_5, act_name_COURSE_SL_6,
act_name_COURSE_WDW_1, act_name_COURSE_WDW_2, act_name_COURSE_WDW_3, act_name_COURSE_WDW_4, act_name_COURSE_WDW_5, act_name_COURSE_WDW_6,
act_name_COURSE_TTM_1, act_name_COURSE_TTM_2, act_name_COURSE_TTM_3, act_name_COURSE_TTM_4, act_name_COURSE_TTM_5, act_name_COURSE_TTM_6,
act_name_COURSE_THI_1, act_name_COURSE_THI_2, act_name_COURSE_THI_3, act_name_COURSE_THI_4, act_name_COURSE_THI_5, act_name_COURSE_THI_6,
act_name_COURSE_TTC_1, act_name_COURSE_TTC_2, act_name_COURSE_TTC_3, act_name_COURSE_TTC_4, act_name_COURSE_TTC_5, act_name_COURSE_TTC_6,
act_name_COURSE_RR_1, act_name_COURSE_RR_2, act_name_COURSE_RR_3, act_name_COURSE_RR_4, act_name_COURSE_RR_5, act_name_COURSE_RR_6,











extra_text_0,
extra_text_1,
extra_text_2,
extra_text_3,
extra_text_4,
extra_text_5,
extra_text_6,
    NULL
};
