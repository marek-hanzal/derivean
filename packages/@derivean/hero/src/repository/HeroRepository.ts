import {type Database}      from "@derivean/orm";
import {lazyOf}             from "@use-pico/container";
import {
    type Client,
    withClient
}                           from "@use-pico/orm";
import {AbstractRepository} from "@use-pico/repository";
import {HeroSchema}         from "../schema/HeroSchema";

export class HeroRepository extends AbstractRepository<
    Database,
    HeroSchema,
    "Hero"
> {
    static inject = [
        lazyOf(withClient.inject),
    ];

    constructor(
        client: Client<Database>,
    ) {
        super(
            client,
            HeroSchema,
            "Hero",
        );
        this.defaultOrderBy = {
            level:    "asc",
            prestige: "asc",
            name:     "asc",
        };
        this.matchOf = {
            userId:    "userId",
            kingdomId: "kingdomId",
        };
    }
}

export namespace HeroRepository {
    export type Type = InstanceType<typeof HeroRepository>;
}
