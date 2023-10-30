import {
    type FilterSchema,
    type IWithMutation
}                    from "@use-pico2/query";
import {DrawerStore} from "@use-pico2/ui";
import {Form}        from "./Form";

export namespace Filter {
    export interface Props<
        TFilterSchema extends FilterSchema
    > extends Omit<
        Form.Props<
            IWithMutation<TFilterSchema, TFilterSchema>
        >,
        "schema" | "onSuccess" | "withMutation"
    > {
        schema: {
            filter: TFilterSchema;
        },
        // withSourceQuery: WithSourceQuery<any, QuerySchema<TFilterSchema, any>>;
    }
}

export const Filter = <TFilterSchema extends FilterSchema>(
    {
        schema: {filter},
        // withSourceQuery,
        ...props
    }: Filter.Props<TFilterSchema>
) => {
    // const query = withSourceQuery.store.use((
    //     {
    //         filter,
    //         setFilter,
    //         isFilter,
    //         clearFilter,
    //         setCursor
    //     }) => ({
    //     filter,
    //     setFilter,
    //     isFilter,
    //     clearFilter,
    //     setCursor
    // }));
    const drawer = DrawerStore.use(({close}) => ({close}));

    return "Filter";

    // return <DrawerStoreProvider>
    //     <WithTranslationProvider
    //         withTranslation={props.withTranslation}
    //     >
    //         <Drawer
    //             drawerId={"filter"}
    //             size={"xl"}
    //             title={"title"}
    //             closeOnClickOutside={false}
    //         >
    //             <Form
    //                 schema={filter}
    //                 submitProps={{
    //                     leftSection: <FilterIcon/>,
    //                 }}
    //                 values={query.filter}
    //                 withAutoClose={["filter"]}
    //                 notification={false}
    //                 onSuccess={async (
    //                     {
    //                         values,
    //                     }) => {
    //                     query.setCursor(0);
    //                     query.setFilter(mapEmptyToUndefined(values));
    //                 }}
    //                 withMutation={withPassMutation({
    //                     key:    ["filter"],
    //                     schema: filter,
    //                 })}
    //                 leftSection={<>
    //                     <ButtonGroup>
    //                         <Button
    //                             variant={"subtle"}
    //                             leftSection={<CloseIcon/>}
    //                             onClick={() => {
    //                                 drawer.close("filter");
    //                             }}
    //                         >
    //                             <Translation namespace={"common.filter"} withLabel={"close.label"}/>
    //                         </Button>
    //                         <Button
    //                             variant={"subtle"}
    //                             leftSection={<FilterOffIcon/>}
    //                             onClick={() => {
    //                                 query.clearFilter();
    //                                 drawer.close("filter");
    //                             }}
    //                         >
    //                             <Translation namespace={"common.filter"} withLabel={"clear.label"}/>
    //                         </Button>
    //                     </ButtonGroup>
    //                 </>}
    //                 {...props}
    //             />
    //         </Drawer>
    //     </WithTranslationProvider>
    //     <WithTranslationProvider
    //         withTranslation={{
    //             namespace: "common.filter",
    //         }}
    //     >
    //         <Group gap={"xs"}>
    //             <DrawerButton
    //                 drawerId={"filter"}
    //                 variant={"subtle"}
    //                 color={query.isFilter() ? "green.7" : undefined}
    //                 leftSection={query.isFilter() ? <FilterOnIcon/> : <FilterOffIcon/>}
    //                 label={"button"}
    //             />
    //             {query.isFilter() && <ActionIcon
    //                 variant={"subtle"}
    //                 onClick={() => {
    //                     query.setCursor(0);
    //                     query.clearFilter();
    //                 }}
    //             >
    //                 <CrossIcon/>
    //             </ActionIcon>}
    //         </Group>
    //     </WithTranslationProvider>
    // </DrawerStoreProvider>;
};