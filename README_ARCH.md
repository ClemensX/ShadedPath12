# Architecture (branch master2)

### Application and Framework Initialization

![Initialization](http://www.plantuml.com/plantuml/proxy?cache=no&src=https://raw.githubusercontent.com/ClemensX/ShadedPath12/s_docu/README_ARCH.md&idx=0)

<details><summary></summary>
```plantuml
@startuml
|Job Service|
|RBSC Backend|
|Repositories|
|SWDC|
    |Job Service|
    start
    ://**/rest/regservice/jobs/visibilityupdate**//
    RegServiceRS.jobs_visibilityupdate();
    |RBSC Backend|
    :JobService.visibilityupdate();
    :VisibilityUpdate.phaseCollect();
        :User 0
        User 1
        User 2
        User 3
        User 4
        ...;
    repeat
    :phaseDivideSWDC();
    fork
        :User 0
        User 10
        User 20
        ...;
    fork again
        :User 1
        User 11
        User 21
        ...;
    fork again
        :User 2
        User 12
        User 22
        ...;
    end fork
    :phaseSWDCCalls();
    |SWDC|
    fork
        :/visibility?userid
        //stack//
        //componentversion//
        //productversion//
        ;
    fork again
        :/visibility?userid
        //stack//
        //componentversion//
        //productversion//
        ;
    fork again
        :/visibility?userid
        //stack//
        //componentversion//
        //productversion//
        ;
    end fork
    |RBSC Backend|
    :wait for all calls to finish;
        :
				
				phaseDivideRepo(info)
				phaseRepoCalls(info)
				phaseUpdateDB(info)
				phaseRemoveUnregisteredPV(info.jobLogger)
				phaseRemoveUnregisteredSCV(info.jobLogger)
        ;
    repeat while (finished ok || try_count > 3 ?) is (no)
    ->yes;
    |Job Service|
    :write AuditLogs;
    :write SystemAlerts;
    stop
@enduml
```
</details>
