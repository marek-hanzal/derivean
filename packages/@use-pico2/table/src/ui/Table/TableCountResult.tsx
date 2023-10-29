import {WithTranslationStore} from "@use-pico2/i18n";
import {
    type FilterSchema,
    type OrderBySchema,
    QuerySchema
}                             from "@use-pico2/query";
import {type PicoSchema}      from "@use-pico2/schema";
import {type FC}              from "react";

export namespace TableCountResult {
    export interface Props<
        TSchema extends PicoSchema,
        TQuerySchema extends QuerySchema<FilterSchema, OrderBySchema>,
    > {
        // withSourceQuery: WithSourceQuery<TSchema, TQuerySchema>;
        Empty?: FC;
    }
}

export const TableCountResult = <
    TSchema extends PicoSchema,
    TQuerySchema extends QuerySchema<FilterSchema, OrderBySchema>,
>(
    {
        // withSourceQuery,
        Empty,
    }: TableCountResult.Props<TSchema, TQuerySchema>
) => {
    const {namespace} = WithTranslationStore.use(({namespace}) => ({namespace}));
    // const countResult = withSourceQuery.useCount();

    return "TableCountResult";

    // const Empty$ = useCallback(() => <Status
    //     title={"empty.title"}
    //     message={"empty.message"}
    // />, []);
    // const WithEmpty = Empty || Empty$;
    //
    // return <>
    //     {countResult.data && !countResult.data.count && countResult.data.where > 0 && <Container size={"md"}>
    //         <Result
    //             icon={<WithIcon
    //                 size={"xl"}
    //                 icon={<IconSearch size={256}/>}
    //             />}
    //             withTranslation={{
    //                 namespace,
    //                 label: "filtered",
    //             }}
    //         />
    //     </Container>}
    //     {countResult.data && !countResult.data.where && <WithEmpty/>}
    // </>;
};
