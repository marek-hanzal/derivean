import {withClient}      from "@derivean/orm";
import {type IContainer} from "@use-pico/container";
import {
    type CountSchema,
    type QuerySchema
}                        from "@use-pico/query";
import {type PicoSchema} from "@use-pico/schema";

export namespace withCount {
    export interface Props<
        TQuerySchema extends QuerySchema<any, any>,
    > {
        container: IContainer.Type;
        request: PicoSchema.Output<TQuerySchema>;
        table: string;
    }
}

export const withCount = async <
    TQuerySchema extends QuerySchema<any, any>,
>(
    {
        request,
        container,
        table,
    }: withCount.Props<TQuerySchema>
): Promise<CountSchema.Type> => {
    const client = withClient.use(container);

    return {
        count: parseInt(
            (await client
                .selectFrom(table as any)
                .select(({fn}) => [
                    fn.count("id").as("count")
                ])
                .executeTakeFirst() as any).count as string
        ),

        where: parseInt(
            (await client
                .selectFrom(table as any)
                .select(({fn}) => [
                    fn.count("id").as("count")
                ])
                .executeTakeFirst() as any).count as string
        ),

        total: parseInt(
            (await client
                .selectFrom(table as any)
                .select(({fn}) => [
                    fn.count("id").as("count")
                ])
                .executeTakeFirst() as any).count as string
        ),
    };
};
