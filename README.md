# ELEC462teamproject

<code>#GAME RULE</code><br>


**[해커팀 VS 유저팀]**<br>
8명의 CHAT&GAME program(마피아게임 변형)<br>
해커 2명(=마피아), 화이트해커 1명(=경찰), V3 1명(=의사), 유저 4명(=시민) : 총 8명이서 게임을 플레이. 해커가 유저 편보다 많거나 같으면 해커팀의 승리. 모든 해커를 찾아 투표로 처형시키면 유저팀의 승리.
1. HACKER : 2명의 해커는 서로를 알 수 없는 채로 플레이어들의 방화벽을 무력화시킨다. 매일 밤이되면 능력을 사용하여 방화벽을 무력화시킬 플레이어를 선택할 수 있다.
(해커가 서로를 지목하여 무력화시키려고 할 경우 먼저 입장한 플레이어가 우선권을 가진다.)
2. WHITEHACKER : 1명의 화이트해커는 매일 밤 플레이어들의 직업을 알아낼 수 있다.
3. V3 : 1명의 V3는 매일 밤 해커들의 공격으로부터 플레이어들을 지켜줄 수 있다. 물론 자신 스스로도 지킬 수 있다.
4. USER : 능력은 없지만 V3,WHITEHACKER와 합심하여 해커를 찾아 투표로 처형시켜야 한다.

<br>
<code>#HOW TO USE</code>
<br>
<br>

**[COMPILE]**
<br>
  1. gcc -o mafia_serv mafia_serv.c -lpthread  : mafia server compile (pthread 라이브러리 추가)<br>
     - command : ./mafia_serv [PORT]<br>
  2. gcc -o mafia_clnt mafia_clnt.c -lpthread : mafia client compile (pthread 라이브러리 추가)<br>
     - command : ./mafia_clnt [IP] [PORT] [NAME]<br>
    *ip address이용에 대한 제약이 있을 시 localhost 사용 권장
                                                  
**[INGAME USAGE]**<br>
플레이어 번호 : 1~8<br>
낮 시간 : 자유로운 채팅
투표 : #[PLAYERNUM]#을 이용하여 플레이어에게 투표가능<br>
투표 확정 : 최후의 변론 이후 #q#(찬성) #b#(반대)를 입력하여 찬반 투표가능<br>
밤 시간 : 직업별로 능력을 사용할 플레이어에게 #[PLAYERNUM]#을 입력하여 능력 사용가능<br>
<br>
<br>
<code>#IMPLEMENTATION ENVIRONMENT</code>
<br>
1. Ubuntu 18.04 LTS (also available in Ubuntu 20.04 LTS)
2. Localhost : 127.0.0.1 , PORT : 8080
3. Language : C

**[NOTICE]**
가끔 server 내 handle_clnt thread에서, night phase에 특정 유저의 socket이 block되어 read()에서 읽어들일 수 없는 현상이 있습니다. <br>
night phase에만 발생하고, 모종의 이유로 socket stream이 block되어 client 입력이 stream에 쌓인 채 전송되지 못한 다는 것은 알 수 있었으나, 이를 해결할 수 없었습니다. <br>
실행환경에 따라 정상작동하는 경우도 있으며, vmware ubuntu 64bit - 18.04 LTS(also 20.04 LTS)에서는 정상작동하는 것을 확인했습니다.
