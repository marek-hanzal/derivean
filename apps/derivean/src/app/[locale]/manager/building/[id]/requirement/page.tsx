import {
    BuildingRequirementQueryStore,
    BuildingRequirementTable,
    withBuildingRepository
}                      from "@derivean/building";
import {BuildingMenu}  from "@derivean/manager";
import {container}     from "@derivean/server";
import {BuildingIcon}  from "@derivean/ui";
import {
    t,
    tx
}                      from "@use-pico/i18n";
import {StoreProvider} from "@use-pico/store";
import {
    Breadcrumbs,
    HomeIcon,
    ListIcon,
    Page
}                      from "@use-pico/ui";

export namespace Index {
    export interface Props {
        params: {
            id: string;
        };
    }
}

export default async function Requirement({params: {id}}: Index.Props) {
    const building = await withBuildingRepository.use(container).withQuery.fetchOrThrow({
        where: {
            id,
        }
    });

    return <Page
        icon={<BuildingIcon/>}
        text={{
            title:  tx({values: building})`Building requirement (title)`,
            header: t({values: building})`Building requirement`,
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
                    href:  "/manager/building/list",
                    label: t()`Building list`,
                    icon:  <ListIcon/>,
                },
            ]}
        />}
        append={<BuildingMenu
            buildingId={building.id}
            active={["/manager/building/[id]/requirement"]}
        />}
    >
        <StoreProvider
            store={BuildingRequirementQueryStore}
            values={{}}
        >
            <BuildingRequirementTable
                hidden={["building"]}
                buildingId={building.id}
            />
        </StoreProvider>
    </Page>;
};
