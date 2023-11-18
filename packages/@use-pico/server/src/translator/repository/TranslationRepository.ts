import {type Infer}        from "@use-pico/extras";
import {
    AbstractRepository,
    type Database,
    lazyOf,
    withConnection
}                          from "@use-pico/server";
import {hashOf}            from "@use-pico/utils";
import {TranslationSchema} from "../schema/TranslationSchema";

export class TranslationRepository extends AbstractRepository<
    Database,
    TranslationSchema,
    "Translation"
> {
    static inject = [
        lazyOf(withConnection.inject),
    ];

    constructor(
        client: withConnection,
    ) {
        super(
            client,
            TranslationSchema,
            "Translation",
        );
        this.defaultOrderBy = {
            locale: "asc",
            key:    "asc",
        };
        this.matchOf = {
            locale: "locale",
            hash:   "hash",
            key:    "key",
        };
    }


    public async toCreate(create: Infer.Create<TranslationSchema>): Promise<Infer.EntityWithoutId<TranslationSchema>> {
        return {
            ...create,
            hash: hashOf(create.key),
        };
    }

    public async toUpdate(update: Infer.Update<TranslationSchema>["update"]): Promise<Infer.Entity$<TranslationSchema>> {
        return {
            ...update,
            hash: update?.key ? hashOf(update.key) : undefined
        };
    }
}

export namespace TranslationRepository {
    export type Type = InstanceType<typeof TranslationRepository>;
}
