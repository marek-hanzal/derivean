import {IconRefresh}             from "@tabler/icons-react";
import {
    type IQueryStore,
    type QuerySchema
}                                from "@use-pico2/query";
import {type WithIdentitySchema} from "@use-pico2/schema";
import {
    Fulltext,
    type IWithSourceQuery,
    useInvalidator
}                                from "@use-pico2/source";
import {
    ActionIcon,
    Grid,
    GridCol
}                                from "@use-pico2/ui";
import {type FC}                 from "react";

export namespace TableHeaderControls {
    export interface Props<
        TQuerySchema extends QuerySchema<any, any>,
        TSchema extends WithIdentitySchema,
    > {
        withQueryStore: IQueryStore<TQuerySchema>;
        withSourceQuery: IWithSourceQuery<TQuerySchema, TSchema>;
        Filter?: FC<FilterProps<TQuerySchema, TSchema>>;
        Postfix?: FC;
    }

    export interface FilterProps<
        TQuerySchema extends QuerySchema<any, any>,
        TSchema extends WithIdentitySchema,
    > {
        withQueryStore: IQueryStore<TQuerySchema>;
        withSourceQuery: IWithSourceQuery<TQuerySchema, TSchema>;
    }
}

export const TableHeaderControls = <
    TQuerySchema extends QuerySchema<any, any>,
    TSchema extends WithIdentitySchema,
>(
    {
        withQueryStore,
        withSourceQuery,
        Filter,
        Postfix,
    }: TableHeaderControls.Props<TQuerySchema, TSchema>
) => {
    const invalidator = useInvalidator({
        withSourceQuery,
    });
    return <Grid
        align={"center"}
        mb={"xs"}
        gutter={"xs"}
    >
        <GridCol span={"auto"}>
            <Fulltext
                withQueryStore={withQueryStore}
            />
        </GridCol>
        <GridCol span={"content"}>
            <ActionIcon
                size={"xl"}
                variant={"subtle"}
                color={"blue.5"}
                onClick={() => invalidator()}
            >
                <IconRefresh/>
            </ActionIcon>
        </GridCol>
        {Filter && <GridCol span={"content"}>
            <Filter
                withQueryStore={withQueryStore}
                withSourceQuery={withSourceQuery}
            />
        </GridCol>}
        {Postfix && <GridCol span={"content"}>
            <Postfix/>
        </GridCol>}
    </Grid>;
};
