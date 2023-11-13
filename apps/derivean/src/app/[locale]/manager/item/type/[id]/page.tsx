import {
    ItemQueryStore,
    ItemTable,
    withItemTypeRepository
}                      from "@derivean/item";
import {container}     from "@derivean/server";
import {ItemIcon}      from "@derivean/ui";
import {t}             from "@use-pico/i18n";
import {StoreProvider} from "@use-pico/store";
import {
    Breadcrumbs,
    HomeIcon,
    ListIcon,
    Page
}                      from "@use-pico/ui";

export namespace List {
    export interface Props {
        params: {
            id: string;
        };
    }
}

export default async function List({params: {id}}: List.Props) {
    const itemType = await withItemTypeRepository.use(container).getOrThrow(id);

    return <Page
        icon={<ItemIcon/>}
        text={{
            header: t({values: itemType})`Item type - item list`,
        }}
        postfix={<Breadcrumbs
            items={[
                {
                    type: "link",
                    href: "/manager",
                    icon: <HomeIcon/>,
                },
                {
                    type:  "link",
                    href:  "/manager/item/type/list",
                    label: t()`Item type list`,
                    icon:  <ListIcon/>,
                },
            ]}
        />}
    >
        <StoreProvider
            store={ItemQueryStore}
            values={{
                where: {
                    typeId: id,
                },
            }}
        >
            <ItemTable
                resourceTypeId={itemType.id}
            />
        </StoreProvider>
    </Page>;
}
