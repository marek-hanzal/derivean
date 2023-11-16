import {type Database}      from "@derivean/orm";
import {
    type IUserService,
    withUserService
}                           from "@use-pico/auth-server";
import {lazyOf}             from "@use-pico/container";
import {withDullSchema}     from "@use-pico/dull-stuff";
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
        lazyOf(withUserService.inject),
    ];

    constructor(
        client: Client<Database>,
        protected userService: IUserService,
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

    public async toCreate(create: withDullSchema.Infer.Create<HeroSchema>): Promise<withDullSchema.Infer.EntityWithoutId<HeroSchema>> {
        return {
            ...create,
            userId: create.userId ?? this.userService.requiredId(),
        };
    }
}

export namespace HeroRepository {
    export type Type = InstanceType<typeof HeroRepository>;
}
