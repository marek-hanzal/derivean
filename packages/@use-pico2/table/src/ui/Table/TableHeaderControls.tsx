import {
    type FilterSchema,
    type OrderBySchema,
    QuerySchema
}                        from "@use-pico2/query";
import {type PicoSchema} from "@use-pico2/schema";
import {type FC}         from "react";

export namespace TableHeaderControls {
    export interface Props<
        TSchema extends PicoSchema,
        TQuerySchema extends QuerySchema<FilterSchema, OrderBySchema>,
    > {
        // withSourceQuery: WithSourceQuery<TSchema, TQuerySchema>;
        Filter?: FC<FilterProps<TSchema, TQuerySchema>>;
        Postfix?: FC;
    }

    export interface FilterProps<
        TSchema extends PicoSchema,
        TQuerySchema extends QuerySchema<FilterSchema, OrderBySchema>,
    > {
        // withSourceQuery: WithSourceQuery<TSchema, TQuerySchema>;
    }
}

export const TableHeaderControls = <
    TSchema extends PicoSchema,
    TQuerySchema extends QuerySchema<FilterSchema, OrderBySchema>,
>(
    {
        // withSourceQuery,
        Filter,
        Postfix,
    }: TableHeaderControls.Props<TSchema, TQuerySchema>
) => {
    return "TableHeaderControls";

    // const invalidator = withSourceQuery.useInvalidator();
    // return <Grid
    //     align={"center"}
    //     mb={"xs"}
    //     gutter={"xs"}
    // >
    //     <Grid.Col span={"auto"}>
    //         <Fulltext
    //             withSourceQuery={withSourceQuery}
    //         />
    //     </Grid.Col>
    //     <Grid.Col span={"content"}>
    //         <ActionIcon
    //             size={"xl"}
    //             variant={"subtle"}
    //             color={"blue.5"}
    //             onClick={() => invalidator()}
    //         >
    //             <IconRefresh/>
    //         </ActionIcon>
    //     </Grid.Col>
    //     {Filter && <Grid.Col span={"content"}>
    //         <Filter withSourceQuery={withSourceQuery}/>
    //     </Grid.Col>}
    //     {Postfix && <Grid.Col span={"content"}>
    //         <Postfix/>
    //     </Grid.Col>}
    // </Grid>;
};
