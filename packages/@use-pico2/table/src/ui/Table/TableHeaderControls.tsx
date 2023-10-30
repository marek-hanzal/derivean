import {IconRefresh}             from "@tabler/icons-react";
import {
    type FilterSchema,
    type IWithSourceQuery,
    type OrderBySchema,
    type QuerySchema
}                                from "@use-pico2/query";
import {type WithIdentitySchema} from "@use-pico2/schema";
import {Fulltext}                from "@use-pico2/source";
import {
    ActionIcon,
    Grid,
    GridCol
}                                from "@use-pico2/ui";
import {type FC}                 from "react";

export namespace TableHeaderControls {
    export interface Props<
        TFilterSchema extends FilterSchema,
        TOrderBySchema extends OrderBySchema,
        TQuerySchema extends QuerySchema<TFilterSchema, TOrderBySchema>,
        TSchema extends WithIdentitySchema,
    > {
        withSourceQuery: IWithSourceQuery<TFilterSchema, TOrderBySchema, TQuerySchema, TSchema>;
        Filter?: FC<FilterProps<TFilterSchema, TOrderBySchema, TQuerySchema, TSchema>>;
        Postfix?: FC;
    }

    export interface FilterProps<
        TFilterSchema extends FilterSchema,
        TOrderBySchema extends OrderBySchema,
        TQuerySchema extends QuerySchema<TFilterSchema, TOrderBySchema>,
        TSchema extends WithIdentitySchema,
    > {
        withSourceQuery: IWithSourceQuery<TFilterSchema, TOrderBySchema, TQuerySchema, TSchema>;
    }
}

export const TableHeaderControls = <
    TFilterSchema extends FilterSchema,
    TOrderBySchema extends OrderBySchema,
    TQuerySchema extends QuerySchema<TFilterSchema, TOrderBySchema>,
    TSchema extends WithIdentitySchema,
>(
    {
        withSourceQuery,
        Filter,
        Postfix,
    }: TableHeaderControls.Props<TFilterSchema, TOrderBySchema, TQuerySchema, TSchema>
) => {
    const invalidator = withSourceQuery.useInvalidator();
    return <Grid
        align={"center"}
        mb={"xs"}
        gutter={"xs"}
    >
        <GridCol span={"auto"}>
            <Fulltext
                withSourceQuery={withSourceQuery}
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
            <Filter withSourceQuery={withSourceQuery}/>
        </GridCol>}
        {Postfix && <GridCol span={"content"}>
            <Postfix/>
        </GridCol>}
    </Grid>;
};
